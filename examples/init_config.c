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
