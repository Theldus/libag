/*
 * MIT License
 *
 * Copyright (c) 2021 Davidson Francis <davidsondfgl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
 *
 */
static worker_t *workers;
static int workers_len;

/**
 *
 */
static int has_ag_init;

/**
 *
 */
int ag_start_workers(void)
{
	int num_cores;
	int i;

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

	/* Initialize workers & mutexes. */
	workers = ag_calloc(workers_len, sizeof(worker_t));
	if (pthread_cond_init(&files_ready, NULL))
		goto err1;
	if (pthread_mutex_init(&print_mtx, NULL))
		goto err2;
	if (opts.stats && pthread_mutex_init(&stats_mtx, NULL))
		goto err3;
	if (pthread_mutex_init(&work_queue_mtx, NULL))
		goto err4;

    /* Start workers and wait for something. */
	for (i = 0; i < num_cores; i++)
	{
		workers[i].id = i;
		int rv = pthread_create(&(workers[i].thread), NULL, &search_file_worker,
			&(workers[i].id));
		if (rv)
			goto err5;
	}

	return (0);

err5:
	pthread_mutex_destroy(&work_queue_mtx);
err4:
	if (opts.stats)
		pthread_mutex_destroy(&stats_mtx);
err3:
	pthread_mutex_destroy(&print_mtx);
err2:
	pthread_cond_destroy(&files_ready);
err1:
	free(workers);
	return (1);
}

/**
 *
 */
int ag_stop_workers(void)
{
	int i;

	/* Whatever the workers are doing, we need to stop them. */
	pthread_mutex_lock(&work_queue_mtx);
		done_adding_files = TRUE;
		pthread_cond_broadcast(&files_ready);
	pthread_mutex_unlock(&work_queue_mtx);

	for (i = 0; i < workers_len; i++)
		if (pthread_join(workers[i].thread, NULL))
			return (1);

	/* Clean resources. */
	pthread_cond_destroy(&files_ready);
	pthread_mutex_destroy(&work_queue_mtx);
	pthread_mutex_destroy(&print_mtx);
	cleanup_ignore(root_ignores);
	free(workers);

	return (0);
}

/**
 *
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

	/* Configure regex stuff. */
	compile_study(&opts.re, &opts.re_extra, opts.query, pcre_opts,
		study_opts);

	return (0);
}

/**
 *
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
		*paths = ag_calloc(sizeof(char *), npaths + 1);
		*base_paths = ag_calloc(sizeof(char *), npaths + 1);

		for (i = 0; i < (size_t)npaths; i++)
		{
			path = ag_strdup(tpaths[i]);
			path_len = strlen(path);

			/* Kill trailing slash. */
			if (path_len > 1 && path[path_len - 1] == '/')
				path[path_len - 1] = '\0';

			(*paths)[i] = path;
#ifdef PATH_MAX
			tmp = ag_malloc(PATH_MAX);
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
		path = ag_strdup(".");
		*paths = ag_malloc(sizeof(char *) * 2);
		*base_paths = ag_malloc(sizeof(char *) * 2);
		(*paths)[0] = path;

#ifdef PATH_MAX
		tmp = ag_malloc(PATH_MAX);
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

/**
 *
 */
int ag_init(void)
{
	set_log_level(LOG_LEVEL_WARN);
	root_ignores = init_ignore(NULL, "", 0);

	out_fd = stdout;

	/* Initialize default options. */
	init_options();

	/* Start workers. */
	if (ag_start_workers())
		return (1);

	has_ag_init = 1;
	return (0);
}

/**
 *
 */
int ag_finish(void)
{
	cleanup_options();
	ag_stop_workers();
	return (0);
}

/**
 *
 */
int ag_search(char *query, int npaths, char **target_paths)
{
	char **base_paths;
	char **paths;
	int i;

	/* Check if libag was initialized. */
	if (!has_ag_init)
		return (1);

	/* Query and valid paths. */
	if (!query || !target_paths)
		return (1);

	base_paths = NULL;
	paths = NULL;

	/* Prepare query. */
	if (opts.query)
		free(query);
	opts.query = ag_strdup(query);
	opts.query_len = strlen(query);

	/* Configure search settings. */
	setup_search();

	/* Prepare our paths and base_paths. */
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
			log_err("Failed to get device information for path %s. Skaipping...",
				paths[i]);
		}
#endif
		search_dir(ig, base_paths[i], paths[i], 0, s.st_dev);
		cleanup_ignore(ig);
	}

	return (0);
}
