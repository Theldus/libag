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

#include <stdio.h>
#include <string.h>
#include <libag.h>

int main(int argc, char **argv)
{
	struct ag_result **results;
	struct ag_config config;
	size_t nresults;

	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s \"regex\" [paths]\n", argv[0]);
		return (1);
	}

	/* 4 workers and enable binary files search. */
	memset(&config, 0, sizeof(struct ag_config));
	config.search_binary_files = 1;
	config.num_workers = 4;

	/* Initiate Ag library with default options. */
	ag_init_config(&config);

	/* Search. */
	results = ag_search(argv[1], argc - 2, argv + 2, &nresults);
	if (!results)
		printf("no result found\n");
	else
		printf("%zu results found\n", nresults);

	/* Show them on the screen, if any. */
	for (size_t i = 0; i < nresults; i++)
	{
		for (size_t j = 0; j < results[i]->nmatches; j++)
		{
			printf("file: %s, match: %s, start: %zu / end: %zu, is_binary: %d\n",
				results[i]->file, results[i]->matches[j]->match,
				results[i]->matches[j]->byte_start,
				results[i]->matches[j]->byte_end,
				!!(results[i]->flags & LIBAG_FLG_BINARY));
		}
	}

	/* Free all resources. */
	ag_free_all_results(results, nresults);

	/* Release Ag resources. */
	ag_finish();
	return (0);
}
