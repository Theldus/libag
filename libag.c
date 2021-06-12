/*
 * Copyright 2021 Davidson Francis <davidsondfgl@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "libag.h"

#include <ctype.h>
#include <pcre.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "config.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "log.h"
#include "options.h"
#include "search.h"
#include "util.h"

typedef struct {
    pthread_t thread;
    int id;
} worker_t;

/**
 * @brief Worker list.
 */
static worker_t *workers = NULL;
static int workers_len;

/**
 * @brief Indicator that Ag is being used as a library.
 */
int has_ag_init;

/**
 * @brief Per-thread result: here contains the partial
 * output that will be returned from ag_search. The
 * idea is that each worker can safely work separately
 * on its own result list that latter will be joined in
 * a single list.
 */
static struct thrd_result
{
	size_t capacity;
	size_t nresults;
	struct ag_result **results;
} thrd_rslt[NUM_WORKERS + 1];

/**
 * In-memory copy of user-defined ag_config.
 *
 * Notes:
 * Although ag_config is almost an alias for Ag's cli_options structure,
 * it's also a good idea to keep an in-memory copy of the user-defined
 * config, since we can add our own options in it that do not depends
 * on the cli_options struct.
 */
static struct ag_config config;

/*
 * @brief Mutex for safe-thread search.
 */
static pthread_mutex_t search_mtx;

/*
 * ============================================================================
 * Util
 * ============================================================================
 */

/**
 * @brief Reset the per-thread result for its default
 * values.
 *
 * @param reset If != 0, allocates new memory for the list,
 *              if 0, do the default behavior.
 *
 * @return Return 0 if success, -1 otherwise.
 */
static int reset_local_results(int reset)
{
	int i;
	for (i = 0; i <= NUM_WORKERS; i++)
	{
		thrd_rslt[i].capacity = 100;
		thrd_rslt[i].nresults = 0;

		if (thrd_rslt[i].results)
		{
			free(thrd_rslt[i].results);
			thrd_rslt[i].results = NULL;
		}

		if (reset)
		{
			thrd_rslt[i].results = calloc(100, sizeof(struct ag_result *));
			if (!thrd_rslt[i].results)
				return (-1);
		}
	}
	return (0);
}

/**
 * @brief For a given number of matches @p matches_len
 * in the current processed file @p file, save the
 * findings in the per-thread result.
 *
 * @param worker_id Current thread.
 * @param file Processed file with the matches found.
 * @param matches Matches list.
 * @param matches_len Matches list length.
 * @param buf File read buffer.
 * @param flags Optional flags, such as binary file indicator.
 *
 * @return Returns 0 if success, -1 otherwise.
 */
int add_local_result(int worker_id, const char *file,
	const match_t matches[], const size_t matches_len,
	const char *buf, int flags)
{
	struct thrd_result *t_rslt;
	struct ag_result **ag_rslt;
	size_t i;
	int idx;

	if (!matches_len)
		return (0);

	t_rslt  = &thrd_rslt[worker_id];
	ag_rslt = t_rslt->results;

	/* Grow. */
	if (t_rslt->nresults >= t_rslt->capacity)
	{
		ag_rslt = realloc(ag_rslt,
			sizeof(struct ag_result *) * (t_rslt->capacity * 2));
		if (!ag_rslt)
			return (-1);

		t_rslt->results   = ag_rslt;
		t_rslt->capacity *= 2;
	}

	idx = t_rslt->nresults;

	/* Allocate and add a new result. */
	ag_rslt[idx] = malloc(sizeof(struct ag_result));
	if (!ag_rslt[idx])
		return (-1);

	ag_rslt[idx]->file = strdup(file);
	if (!ag_rslt[idx]->file)
		return (-1);

	ag_rslt[idx]->nmatches = matches_len;
	ag_rslt[idx]->flags = flags;

	/* Allocate and adds the matches into the matches list. */
	ag_rslt[idx]->matches = malloc(sizeof(struct ag_match *) * (matches_len+1));
	if (!ag_rslt[idx]->matches)
		return (-1);

	for (i = 0; i < matches_len; i++)
	{
		ag_rslt[idx]->matches[i] = malloc(sizeof(struct ag_match));
		ag_rslt[idx]->matches[i]->byte_start = matches[i].start;
		ag_rslt[idx]->matches[i]->byte_end = matches[i].end - 1;

		/* Reserve space for the match string and copy. */
		ag_rslt[idx]->matches[i]->match = calloc(1,
			sizeof(char) * ((matches[i].end - matches[i].start) + 1));

		if (!ag_rslt[idx]->matches[i]->match)
			return (-1);

		memcpy(ag_rslt[idx]->matches[i]->match, buf + matches[i].start,
			(matches[i].end - matches[i].start));
	}
	ag_rslt[idx]->matches[matches_len] = NULL;

	t_rslt->nresults++;
	return (0);
}

/**
 * @brief Join all the per-thread results into a single list
 * and returns it.
 *
 * @param nresults_ret Number of results pointer.
 *
 * @return Returns all the results found.
 */
static struct ag_result **get_thrd_results(size_t *nresults_ret)
{
	struct ag_result **rslt;
	size_t nresults;
	size_t i, j, idx;

	rslt = NULL;
	nresults = 0;

	/* Get the results amount. */
	for (i = 0; i <= NUM_WORKERS; i++)
		if (thrd_rslt[i].nresults)
			nresults += thrd_rslt[i].nresults;

	/* If nothing is found, return NULL. */
	if (!nresults)
		goto out;

	/* Allocate results. */
	idx  = 0;
	rslt = malloc(sizeof(struct ag_result *) * (nresults+1));
	if (!rslt)
		goto out;

	/* Add the results. */
	for (i = 0; i <= NUM_WORKERS; i++)
		if (thrd_rslt[i].nresults)
			for (j = 0; j < thrd_rslt[i].nresults; j++)
				rslt[idx++] = thrd_rslt[i].results[j];
	rslt[idx] = NULL;
out:
	*nresults_ret = nresults;
	return (rslt);
}

/**
 * @brief Configure Ag for a new search.
 *
 * This should be invoked at each new search.
 *
 * @return Returns 0 if success, -1 otherwise.
 */
static int setup_search(void)
{
	int study_opts;
	int pcre_opts;

	study_opts = 0;
	pcre_opts  = PCRE_MULTILINE;

	/* Enable JIT if possible. */
#ifdef USE_PCRE_JIT
	int has_jit = 0;
	pcre_config(PCRE_CONFIG_JIT, &has_jit);
	if (has_jit)
		study_opts |= PCRE_STUDY_JIT_COMPILE;
#endif

	/* If smart case. */
	if (opts.casing == CASE_SMART)
	{
		opts.casing = is_lowercase(opts.query) ?
			CASE_INSENSITIVE : CASE_SENSITIVE;
	}

	/* Check if regex. */
	if (!is_regex(opts.query))
		opts.literal = 1;

	if (opts.literal)
	{
		if (opts.casing == CASE_INSENSITIVE)
		{
			/* Search routine needs the query to be lowercase */
			char *c = opts.query;
			for (; *c != '\0'; ++c)
				*c = (char)tolower(*c);
		}
		generate_alpha_skip(opts.query, opts.query_len, alpha_skip_lookup,
			opts.casing == CASE_SENSITIVE);
		find_skip_lookup = NULL;
		generate_find_skip(opts.query, opts.query_len, &find_skip_lookup,
			opts.casing == CASE_SENSITIVE);
		generate_hash(opts.query, opts.query_len, h_table,
			opts.casing == CASE_SENSITIVE);
    }

    /* Regex. */
    else
    {
    	if (opts.casing == CASE_INSENSITIVE)
			pcre_opts |= PCRE_CASELESS;

		/* Configure regex stuff. */
		compile_study(&opts.re, &opts.re_extra, opts.query, pcre_opts,
			study_opts);
	}

	return (0);
}

/**
 * @brief For a given list of paths @p tpaths, generates a list
 * of paths and base paths.
 *
 * @param npaths Number of paths in @p tpaths.
 * @param tpaths Paths to be searched.
 * @param base_path Returned base paths pointer.
 * @param paths Returned paths pointer.
 *
 * @return Returns 0 if success, -1 otherwise.
 */
static int prepare_paths(int npaths, char **tpaths, char **base_paths[],
	char **paths[])
{
	int base_path_len;
	char *base_path;
	int path_len;
	char *path;
	size_t i;

#ifdef PATH_MAX
	char *tmp = NULL;
#endif

	base_path_len = 0;
	path_len      = 0;
	base_path     = NULL;
	path          = NULL;

	/* Multiples files and folders. */
	if (npaths > 0)
	{
		*paths = calloc(sizeof(char *), npaths + 1);
		if (!*paths)
			return (-1);

		*base_paths = calloc(sizeof(char *), npaths + 1);
		if (!*base_paths)
			return (-1);

		for (i = 0; i < (size_t)npaths; i++)
		{
			path = strdup(tpaths[i]);
			if (!path)
				return (-1);

			path_len = strlen(path);

			/* Kill trailing slash. */
			if (path_len > 1 && path[path_len - 1] == '/')
				path[path_len - 1] = '\0';

			(*paths)[i] = path;
#ifdef PATH_MAX
			tmp = malloc(PATH_MAX);
			if (!tmp)
				return (-1);

			base_path = realpath(path, tmp);
#else
			base_path = realpath(path, NULL);
#endif
			if (base_path)
			{
				base_path_len = strlen(base_path);

				/* Add trailing slash. */
				if (base_path_len > 1 && base_path[base_path_len - 1] != '/')
				{
					base_path = ag_realloc(base_path, base_path_len + 2);
					base_path[base_path_len] = '/';
					base_path[base_path_len + 1] = '\0';
				}
			}
			(*base_paths)[i] = base_path;
		}

        /* Make sure we search these paths instead of stdin. */
		opts.search_stream = 0;
	}

	/* Use current folder. */
	else
	{
		path = strdup(".");
		if (!path)
			return (-1);
		*paths = malloc(sizeof(char *) * 2);
		if (!*paths)
			return (-1);

		*base_paths = malloc(sizeof(char *) * 2);
		if (*base_paths)
			return (-1);

		(*paths)[0] = path;

#ifdef PATH_MAX
		tmp = malloc(PATH_MAX);
		if (!tmp)
			return (-1);
		(*base_paths)[0] = realpath(path, tmp);
#else
		(*base_paths)[0] = realpath(path, NULL);
#endif
		i = 1;
	}

	(*paths)[i] = NULL;
    (*base_paths)[i] = NULL;

    /* Set paths len. */
    opts.paths_len = npaths;

    return (0);
}

/*
 * ============================================================================
 * Library exported routines
 * ============================================================================
 */

/**
 * @brief Sets the behavior by an user-specified configuration
 * @p ag_config.
 *
 * @param ag_config User-defined configuration.
 *
 * @return Returns 0 if success, -1 otherwise.
 */
int ag_set_config(struct ag_config *ag_config)
{
	if (!ag_config)
		return (-1);

	opts.literal = ag_config->literal;
	opts.recurse_dirs = !ag_config->disable_recurse_dir;
	if (ag_config->casing < 0 || ag_config->casing > 2)
		return (-1);
	opts.casing = ag_config->casing;
	if (ag_config->num_workers < 0 || ag_config->num_workers > NUM_WORKERS)
		return (-1);
	opts.workers = ag_config->num_workers;
	opts.stats = ag_config->stats;
	opts.search_binary_files = ag_config->search_binary_files;
	memcpy(&config, ag_config, sizeof(struct ag_config));
	return (0);
}

/**
 * @brief Initializes libag with default settings.
 *
 * @return Returns 0 if success, -1 otherwise.
 */
int ag_init(void)
{
	ag_init_config(NULL);
	return (0);
}

/**
 * @brief Initializes libag with an user-specified
 * configuration @p config.
 *
 * @param config User-defined configuration.
 *
 * @return Returns 0 if success, -1 otherwise.
 */
int ag_init_config(struct ag_config *ag_config)
{
	/* Reset if already initiated. */
	if (has_ag_init)
		ag_finish();

	set_log_level(LOG_LEVEL_WARN);
	root_ignores = init_ignore(NULL, "", 0);

	out_fd = stdout;

	/* Initialize default options. */
	init_options();

	/* User-defined options, if none, use default settings. */
	ag_set_config(ag_config);

	/* Start workers. */
	if (config.workers_behavior == LIBAG_START_WORKERS && ag_start_workers())
		return (-1);

	has_ag_init = 1;
	return (0);
}

/**
 * @brief Finishes, i.e, releases all* resources belonging
 * to Ag/libag.
 *
 * @return Returns 0 if success, -1 otherwise.
 *
 * @note *It is up to the user to release all the memory
 * relative to the results found. This memory can be
 * released with the use of the helper functions @ref
 * ag_free_result and @ref ag_free_all_results. For more
 * 'fine-grained' memory release, the user can manually
 * iterate over the structures and release with 'free'.
 */
int ag_finish(void)
{
	cleanup_options();
	ag_stop_workers();
	has_ag_init = 0;
	return (0);
}


/**
 * @brief Start the worker threads for libag.
 *
 * This is called by @ref ag_init and there is no need
 * to call this manually. However, a more experienced
 * may want to control whether to start/stop the worker
 * threads. By default, all worker threads will be
 * spawned at @ref ag_init and deleted with @ref ag_finish.
 *
 * If a user wants to call @ref ag_search multiples times,
 * maybe it is interesting to keep the workers all the time.
 *
 * @return Returns 0 if success, -1 otherwise.
 */
int ag_start_workers(void)
{
	int num_cores;
	int i;

	/* Check if there are workers already. */
	if (workers)
		return (-1);

	workers = NULL;
	work_queue = NULL;
	work_queue_tail = NULL;

#ifdef _WIN32
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		num_cores = si.dwNumberOfProcessors;
	}
#else
	num_cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif

	/* Set worker length. */
	workers_len = num_cores < 8 ? num_cores : 8;
	if (opts.literal)
		workers_len--;
	if (opts.workers)
		workers_len = opts.workers;
	if (workers_len < 1)
		workers_len = 1;

	done_adding_files = FALSE;
	stop_workers = FALSE;

	/* Initialize workers & mutexes. */
	workers = calloc(workers_len, sizeof(worker_t));
	if (!workers)
		return (-1);
	if (pthread_cond_init(&files_ready, NULL))
		goto err1;
	if (pthread_mutex_init(&print_mtx, NULL))
		goto err2;
	if (pthread_mutex_init(&stats_mtx, NULL))
		goto err3;
	if (pthread_mutex_init(&work_queue_mtx, NULL))
		goto err4;
	if (pthread_mutex_init(&search_mtx, NULL))
		goto err5;
	if (pthread_barrier_init(&worker_done, NULL, workers_len + 1))
		goto err6;
	if (pthread_barrier_init(&results_done, NULL, workers_len + 1))
		goto err7;

	/* Reset per-thread local results. */
	if (reset_local_results(1))
		goto err8;

    /* Start workers and wait for something. */
	for (i = 0; i < workers_len; i++)
	{
		workers[i].id = i;
		int rv = pthread_create(&(workers[i].thread), NULL, &search_file_worker,
			&(workers[i].id));

		/* Stop gracefully already started threads. */
		if (rv)
		{
			workers_len = i + 1;
			ag_stop_workers();
			return (-1);
		}
	}
	return (0);
err8:
	pthread_barrier_destroy(&results_done);
err7:
	pthread_barrier_destroy(&worker_done);
err6:
	pthread_mutex_destroy(&search_mtx);
err5:
	pthread_mutex_destroy(&work_queue_mtx);
err4:
	pthread_mutex_destroy(&stats_mtx);
err3:
	pthread_mutex_destroy(&print_mtx);
err2:
	pthread_cond_destroy(&files_ready);
err1:
	free(workers);
	return (-1);
}

/**
 * @brief Stops all workers and cleanup resources.
 *
 * @return Returns 0 if success, -1 otherwise.
 */
int ag_stop_workers(void)
{
	int i;

	/* Check if there are workers to stop. */
	if (!workers)
		return (-1);

	/* Whatever the workers are doing, we need to stop them. */
	pthread_mutex_lock(&work_queue_mtx);
		done_adding_files = TRUE;
		stop_workers = TRUE;
		pthread_cond_broadcast(&files_ready);
	pthread_mutex_unlock(&work_queue_mtx);

	for (i = 0; i < workers_len; i++)
		if (pthread_join(workers[i].thread, NULL))
			return (-1);

	/* Clean resources. */
	pthread_cond_destroy(&files_ready);
	pthread_mutex_destroy(&work_queue_mtx);
	pthread_mutex_destroy(&print_mtx);
	pthread_mutex_destroy(&stats_mtx);
	pthread_mutex_destroy(&search_mtx);
	pthread_barrier_destroy(&worker_done);
	pthread_barrier_destroy(&results_done);
	cleanup_ignore(root_ignores);
	reset_local_results(0);
	free(workers);
	workers = NULL;

	return (0);
}

/**
 * @brief Searches for @p query recursively in all @p target_paths.
 *
 * Thats the main routine for libag and the one that the users want
 * to use.
 *
 * @param query Pattern to be searched.
 * @param npaths Number of paths to be searched.
 * @param target_paths Paths list.
 * @param nresults Pointer to number of results found.
 *
 * @return Returns a list of (struct ag_result*) containing all
 * the results found. Please note that this result is up to the
 * user to free, with @ref ag_free_result and @ref ag_free_all_results.
 *
 * If nothing is found, return NULL.
 *
 * @note Please note that this routine is _not_ thread-safe, and should
 * not be called from multiples threads.
 */
struct ag_result **ag_search(char *query, int npaths, char **target_paths,
	size_t *nresults)
{
	struct ag_result **result;
	char **base_paths;
	char **paths;
	int i;

	result = NULL;
	*nresults = 0;

	/* Check if libag was initialized. */
	if (!has_ag_init)
		return (NULL);

	/* Query and valid paths. */
	if (!query || !target_paths)
		return (NULL);

	/* Check if workers already started or I should start them. */
	if (!workers)
	{
		if (config.workers_behavior != LIBAG_ONSEARCH_WORKERS)
			return (NULL);
		if (ag_start_workers())
			return (NULL);
	}

	/* Reset stats. */
	if (opts.stats)
		memset(&stats, 0, sizeof(stats));

	/* Prepare query. */
	free(opts.query);
	opts.query = strdup(query);
	if (!opts.query)
		goto err1;
	opts.query_len = strlen(query);

	/* Configure search settings. */
	opts.casing = config.casing;
	setup_search();

	/* Prepare our paths and base_paths. */
	base_paths = NULL;
	paths = NULL;
	prepare_paths(npaths, target_paths, &base_paths, &paths);

	/* Search everything. */
	for (i = 0; paths[i] != NULL; i++)
	{
		log_debug("searching path %s for %s", paths[i], opts.query);
		symhash = NULL;
		ignores *ig = init_ignore(root_ignores, "", 0);
		struct stat s = { .st_dev = 0 };

#ifndef _WIN32
		/*
		 * The device is ignored if opts.one_dev is false, so it's fine
		 * to leave it at the default 0.
		 */
		if (opts.one_dev && lstat(paths[i], &s) == -1)
		{
			log_err("Failed to get device information for path %s. Skipping...",
				paths[i]);
		}
#endif
		search_dir(ig, base_paths[i], paths[i], 0, s.st_dev);
		cleanup_ignore(ig);
	}

	/* Wakeup threads and let them known that work is done. */
	pthread_mutex_lock(&work_queue_mtx);
		done_adding_files = TRUE;
		pthread_cond_broadcast(&files_ready);
	pthread_mutex_unlock(&work_queue_mtx);

	/* Wait to complete. */
	pthread_barrier_wait(&worker_done);

	/* Work. */
	result = get_thrd_results(nresults);

	/* Reset & wakeup workers again to wait for more work. */
	reset_local_results(1);
	done_adding_files = FALSE;
	pthread_barrier_wait(&results_done);

	/* Cleanup paths. */
	for (i = 0; paths[i] != NULL; i++)
	{
		free(paths[i]);
		free(base_paths[i]);
	}
	free(base_paths);
	free(paths);
	if (find_skip_lookup)
		free(find_skip_lookup);

	/* Cleanup query. */
	free(opts.query);
	opts.query = NULL;

err1:
	/* Stop workers, if necessary. */
	if (workers && config.workers_behavior == LIBAG_ONSEARCH_WORKERS)
		if (ag_stop_workers())
			return (NULL);

	return (result);
}

/**
 * @Brief Searches for @p query recursively in all @p target_paths.
 *
 * This is the same behavior as the @ref ag_search routine, but thread-safe.
 *
 * @param query Pattern to be searched.
 * @param npaths Number of paths to be searched.
 * @param target_paths Paths list.
 * @param nresults Pointer to number of results found.
 *
 * @return Returns a list of (struct ag_result*) containing all
 * the results found. If nothing found, NULL.
 *
 * @note Please note that this version is the same as ag_search() but
 * thread-safe.
 *
 * Also note that the thread-safe version here is the most rudimentary
 * implementation possible: just one lock on the entire function. Which
 * implies that parallel calls to this function will not be parallel, but
 * sequential.
 *
 * A more efficient implementation is needed. See the issues on GitHub for a
 * breakdown of this.
 */
struct ag_result **ag_search_ts(char *query, int npaths, char **target_paths,
	size_t *nresults)
{
	struct ag_result **r;

	/* Check if libag was initialized. */
	if (!has_ag_init)
		return (NULL);

	/* Query and valid paths. */
	if (!query || !target_paths || !nresults || npaths <= 0)
		return (NULL);

	/* Check if workers already started or I should start them. */
	if (!workers)
	{
		if (config.workers_behavior != LIBAG_ONSEARCH_WORKERS)
			return (NULL);
	}

	/* Lock search. */
	pthread_mutex_lock(&search_mtx);
		r = ag_search(query, npaths, target_paths, nresults);
	pthread_mutex_unlock(&search_mtx);
	return (r);
}

/**
 * @brief If stats are enabled, get the current stats for
 * the latest @ref ag_search call.
 *
 * @param ret_stats Stats structure to be filled.
 *
 * @return Returns 0 if success, -1 otherwise.
 */
int ag_get_stats(struct ag_search_stats *ret_stats)
{
	if (!config.stats)
		return (-1);
	ret_stats->total_bytes = stats.total_bytes;
	ret_stats->total_files = stats.total_files;
	ret_stats->total_matches = stats.total_matches;
	ret_stats->total_file_matches = stats.total_file_matches;
	return (0);
}

/**
 * @brief For a given @p result, free a single result from libag.
 *
 * @param result Single result.
 */
void ag_free_result(struct ag_result *result)
{
	size_t i;
	if (!result)
		return;
	for (i = 0; i < result->nmatches; i++)
	{
		free(result->matches[i]->match);
		free(result->matches[i]);
	}
	free(result->matches);
	free(result->file);
	free(result);
}

/**
 * @brief For a given @p results and @p nresults, free all
 * results returned from @ref ag_search.
 *
 * @param results Results list to be freed.
 * @param nresults Results list length.
 */
void ag_free_all_results(struct ag_result **results, size_t nresults)
{
	size_t i;
	if (!results || !nresults)
		return;
	for (i = 0; i < nresults; i++)
		ag_free_result(results[i]);
	free(results);
}
