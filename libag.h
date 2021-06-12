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

#ifndef LIBAG_H
#define LIBAG_H

	#include <stddef.h>

	/* Tag/Release identifier. */
	#define TAG_ID "v2-apache_license"

	/* Num workers. */
	#define NUM_WORKERS 8

	/* Casing. */
	#define LIBAG_CASE_SMART       0
	#define LIBAG_CASE_SENSITIVE   1
	#define LIBAG_CASE_INSENSITIVE 2

	/* Workers behavior. */
	#define LIBAG_START_WORKERS    0
	#define LIBAG_MANUAL_WORKERS   1
	#define LIBAG_ONSEARCH_WORKERS 2

	/* Result flags. */
	#define LIBAG_FLG_TEXT   1
	#define LIBAG_FLG_BINARY 2

	/**
	 * Structure that holds a single result, i.e: a file
	 * that may contains multiples matches.
	 */
	struct ag_result
	{
		char *file;
		size_t nmatches;
		struct ag_match
		{
			size_t byte_start;
			size_t byte_end;
			char *match;
		} **matches;
		int flags;
	};

	/**
	 * @brief libag search stats
	 *
	 * Holds stats relative to the latest search performed by @ref
	 * ag_search.
	 *
	 * You can get this structure by enabling stats via ag_config,
	 * and retrieving later with @ref ag_get_stats.
	 *
	 * Note:
	 * This is also very similar to the 'ag_stats' structure but
	 * without the time fields. The reasoning behind the usage of
	 * this instead of 'ag_stats' is the same of the structure
	 * below.
	 */
	struct ag_search_stats
	{
		size_t total_bytes;        /* Amount of bytes read.            */
		size_t total_files;        /* Amount of files read.            */
		size_t total_matches;      /* Amount of matches.               */
		size_t total_file_matches; /* Amount of files that have match. */
	};

	/**
	 * @brief libag configuration structure.
	 *
	 * This structure holds the configuration that would be used inside
	 * Ag, you're free to use this structure inside your program and
	 * set whatever config you want.
	 *
	 * Note 1:
	 * -------
	 * Please note that whenever you change some field here, please
	 * also call the routine @ref ag_set_config, otherwise, the
	 * changes will not be reflected.
	 *
	 * Note 2:
	 * -------
	 * Beware with trash data inside this structure, do a memset first
	 * or allocate with calloc.
	 *
	 * Note 3:
	 * -------
	 * Zeroed options is the default behavior of ag_init()!.
	 *
	 * Technical notes:
	 * ----------------
	 * This structure is very similar to the 'cli_options' struct
	 * that Ag uses internally. One could say that I should use
	 * 'cli_options' instead of this one, but here are my thoughts
	 * about this:
	 *
	 * - This header do _not_ exposes internal Ag functionality to the
	 * user program and modify Ag in order to expose cli_options seems
	 * wrong to me. The core idea of libag is to allow the user to use
	 * libag with only this header (libag.h) and the library (libag.so).
	 *
	 * - The 'cli_options' structure:
	 *   - Holds some data that are never exposed to the command-line, but
	 *     only used internally; mix internal/external only confuses the
	 *     user.
	 *
	 *   - Contains a lot of options that: a) will not be supported by
	 *     libag, such as: --color,--context,--after,--before and etc.
	 *     b) are not supported at the moment; allows the user to
	 *     mistakenly think that these options are supported do not
	 *     seems right to me.
	 *
	 */
	struct ag_config
	{
		/* != 0 enable literal search, 0 disable (default) (i.e: uses regex). */
		int literal;
		/* != 0 disable folder recursion, 0 enables (default). */
		int disable_recurse_dir;
		/*
		 * Casing:
		 * 0 (smart case, default): if pattern is all lowercase, do a case
		 *                          insensitive search, otherwise, case
		 *                          sensitive.
		 * 1 (case sensitive).
		 * 2 (case insensitive).
		 */
		int casing;
		/*
		 * Number of concurrently workers (threads) running at the same time.
		 *
		 * 0 (default): uses the amount of cores available (if <= NUM_WORKERS),
		 *              or NUM_WORKERS (if > NUM_WORKERS).
		 *
		 * Or:
		 * 1 <= num_workers <= NUM_WORKERS.
		 */
		int num_workers;
		/*
		 * By default, libag always start worker threads after a successful
		 * call to ag_init, however, a user may want to have a fine control
		 * over the workers, whether by starting/stopping by hand (via
		 * ag_start_workers/ag_stop_workers or by letting ag_seach
		 * manages the start/stop).
		 *
		 * LIBAG_START_WORKERS    0 - start workers on ag_init (default)
		 * LIBAG_MANUAL_WORKERS   1 - manual workers management.
		 * LIBAG_ONSEARCH_WORKERS 2 - start/stop workers on every ag_search.
		 */
		int workers_behavior;
		/*
		 * Enable a few stats for the last search.
		 */
		int stats; /* 0 disable (default), != 0 enable. */
		/*
		 * Search binary files.
		 */
		int search_binary_files; /* 0 disable (default), != 0 enable. */
	};

	/* Library forward declarations. */
	extern int ag_start_workers(void);
	extern int ag_stop_workers(void);
	extern int ag_set_config(struct ag_config *ag_config);
	extern int ag_init(void);
	extern int ag_init_config(struct ag_config *config);
	extern int ag_finish(void);
	extern struct ag_result **ag_search(char *query, int npaths,
		char **target_paths, size_t *nresults);
	extern struct ag_result **ag_search_ts(char *query, int npaths,
		char **target_paths, size_t *nresults);
	extern int ag_get_stats(struct ag_search_stats *ret_stats);
	extern void ag_free_result(struct ag_result *result);
	extern void ag_free_all_results(struct ag_result **results,
		size_t nresults);

#endif /* LIBAG_H. */
