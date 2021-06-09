# libag - The Silver Searcher _Library_ 📚
[![License: Apache v2](https://img.shields.io/badge/license-Apache%20v2-orange)](https://opensource.org/licenses/Apache-2.0)

libag - The famous The Silver Searcher, but library

## Introduction
A few weeks ago, a friend asked me if I knew any tool for recursive regular
expression search in text and binary files. Ag immediately came to my mind,
but unfortunately, [ag(1)](https://github.com/ggreer/the_silver_searcher) is a
program, not a library.

While not impossible, parsing the Ag output would be a bit of a headache, plus
spawning a new process for every search sounds tedious. Similar tools like
[ripgrep(1)](https://github.com/BurntSushi/ripgrep) can output in JSON format,
which certainly makes it a lot easier, but we're still talking about spawning
processes and parsing outputs.

That's how libag was born. Libag allows you to use the ag search engine (and its
facilities), but in the right way (or nearly so).

## Usage & Features
Libag is intended to be as simple as possible and therefore divides the search
process into three simple steps:
1. Initialize all internal ag structures
(via `ag_init()`)

2. Perform as many searches as you like (via `ag_search()`).

3. Clean up the resources (via `ag_finish()`).

Custom search settings are done via `ag_init_config()` and `ag_set_config()`.
The results are a list of struct ag_result*, which contains the file, a list of
matches (containing the match and file offsets corresponding to the match), and
flags.

### A minimal example:
(Complete examples can be found in
[examples/](https://github.com/Theldus/libag/tree/master/examples))
```c
#include <libag.h>

...
struct ag_result **results;
size_t nresults;

char *query    = "foo";
char *paths[1] = {"."};

/* Initiate Ag library with default options. */
ag_init();

/* Searches for foo in the current path. */
results = ag_search(query, 1, paths, &nresults);
if (!results) {
    printf("No result found\n");
    return (1);
}

printf("%zu results found\\n", nresults);

/* Show them on the screen, if any. */
for (size_t i = 0; i < nresults; i++) {
    for (size_t j = 0; j < results[i]->nmatches; j++) {
        printf("file: %s, match: %s\n",
            results[i]->file, results[i]->matches[j]->match,
    }
}

/* Free all results. */
ag_free_all_results(results, nresults);

/* Release Ag resources. */
ag_finish();
...
```

Libag intends to support all features (work in progress) of ag (or at least,
those that make sense for a library). In addition, it allows detailed control
over worker threads, via `ag_start_workers()` and `ag_stop_workers()` (see
[docs](https://github.com/Theldus/libag#documentation) for more details).

## Bindings
Libag has (experimental) bindings support to other programming languages
(currently just Python). For more information and more detailed documentation, see
[bindings/python](https://github.com/Theldus/libag/tree/master/bindings/python).

## Building/Installing
### Dependencies
Libag requires the same dependencies as ag: c99 compiler and libraries: zlib,
lzma, and pcre. These libraries can be installed one by one or by your package
manager.

For Debian-like distributions, something like:
```bash
$ sudo apt install libpcre3-dev zlib1g-dev liblzma-dev
```
(or follow the Ag recommendations
[here](https://github.com/ggreer/the_silver_searcher/blob/a61f1780b64266587e7bc30f0f5f71c6cca97c0f/README.md#building-master))

### Building from source
Once the dependencies are resolved, clone the repository and build. Libag
supports Makefile and CMake. Choose the one that best suits your needs:

 ```bash
 $ git clone https://github.com/Theldus/libag.git
 $ cd libag/
 ```

 #### Makefile
 ```bash
 $ make -j4

 # Optionally (if you want to install):
 $ make install # (PREFIX and DESTDIR allowed here, defaults to /usr/local/)
 ```

 #### CMake
 ```bash
 $ mkdir build/ && cd build/
 $ cmake .. -DCMAKE_BUILD_TYPE=Release
 $ make -j4

 # Optionally (if you want to install):
 $ make install
 ```

 ## Documentation
Detailed documentation of each available routine can be found on the
[man-pages](https://github.com/Theldus/libag/tree/master/doc/man3).
Also, the source code is extensively commented
([libag.h](https://github.com/Theldus/libag/blob/master/libag.h) is a
must-read!). Documentation on bindings and examples can be found
[here](https://github.com/Theldus/libag/tree/master/bindings/python).

Complete examples are also available in the
[examples/](https://github.com/Theldus/libag/tree/master/examples) folder
and are automatically built together with the library for your
convenience ;-).

## Contributing
Libag is always open to the community and willing to accept contributions,
whether with issues, documentation, testing, new features, bug fixes, typos,
etc. Welcome aboard.

## License
Libag is licensed under Apache v2 license.
