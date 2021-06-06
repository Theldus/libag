#!/usr/bin/env python

# MIT License
#
# Copyright (c) 2021 Davidson Francis <davidsondfgl@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import sys
sys.path.append("..")
from libag import *

if len(sys.argv) < 3:
	sys.stderr.write("Usage: {} \"regex\" [paths]\n".format(sys.argv[0]))
	sys.exit(1)

# 4 workers and enable binary files search
config = ag_config()
config.search_binary_files = 1
config.num_workers = 4

# Initiate Ag library with default options.
ag_init_config(config)

# Search.
nresults, results = ag_search(sys.argv[1], sys.argv[2:])

if nresults == 0:
	print("no result found")
else:
	print("{} results found".format(nresults))

	# Show them on the screen, if any.
	for file in results:
		for match in file.matches:
			print("file: {}, match: {}, start: {} / end: {}, is_binary: {}".
				format(file.file, match.match, match.byte_start, match.byte_end,
				(not not file.flags & LIBAG_FLG_BINARY)))

# Free all resources.
if nresults:
	ag_free_all_results(results)

# Release Ag resources.
ag_finish()
