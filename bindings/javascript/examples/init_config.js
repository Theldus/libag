#!/usr/bin/env node

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

const libag = require('../build/Release/libag_wrapper');

if (process.argv.length < 4)
{
	console.log("Usage: " + process.argv[1] + " \"regex\" [paths]");
	process.exit(1);
}

/* 4 workers and enable binary search. */
config = libag.ag_config();
config.search_binary_files = 1;
config.num_workers = 4;

/* Initiate Ag library with default options .*/
libag.ag_init_config(config);

/* Search. */
r = libag.ag_search(process.argv[2], process.argv.slice(3));

if (r === undefined)
	console.log("no result found");
else
{
	console.log(r.nresults + " results found");

	/* Show them on the screen if any. */
	r.results.forEach((file, i) => {
		file.matches.forEach((match, j) => {
			console.log(
				"file: " + file.file + ", match: " + match.match +
				", start: " + match.byte_start + " / end: " + match.byte_end +
				", is_binary: " + !!(file.flags & 2));
		});
	});
}

/* Release Ag resources. */
libag.ag_finish();
