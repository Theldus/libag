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

#ifndef LIBAG_H
#define LIBAG_H

	#include <stddef.h>

	/* Num workers. */
	#define NUM_WORKERS 8

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
	};

	/* Library forward declarations. */
	extern int ag_start_workers(void);
	extern int ag_stop_workers(void);
	extern int ag_init(void);
	extern int ag_finish(void);
	extern struct ag_result **ag_search(char *query, int npaths,
		char **target_paths, size_t *nresults);
	extern void ag_free_result(struct ag_result *result);
	extern void ag_free_all_results(struct ag_result **results,
		size_t nresults);

#endif /* LIBAG_H. */
