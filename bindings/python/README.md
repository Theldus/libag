# Python bindings

## Introduction
Libag has a (very) experimental bindings support for Python 2/3.

All the magic is done with the help of [SWIG](http://www.swig.org/), a program
that, once an interface is specified, is able to generate a 'glue code'
that is then built into a dynamic library to be loaded by CPython.
It's an amazing kickstart for the
['traditional method'](https://docs.python.org/3.7/extending/extending.html).

Personally speaking, it seemed simpler to use than ctypes: a lot already works
out of the box, and the few situations that need special care can be handled via
type maps, for example. Also, loading a dynamic library makes it possible to get
the best out of libag performance in Python!. Furthermore, SWIG also allows
the creation of bindings for other programming languages too =).

## Building
The build process of the Python module can be divided into two simple steps:

### 1) Download & Build SWIG
Download the latest version (or
[v4.0.2](http://prdownloads.sourceforge.net/swig/swig-4.0.2.tar.gz), tested here)
and compile as usual. Optionally you can also install.

Something like:
```bash
cd /path/to/swig
wget http://prdownloads.sourceforge.net/swig/swig-4.0.2.tar.gz
tar xf swig-*.tar.gz
cd swig-*

# If you want to install system-wide
./configure
make -j4
make install

# If you want to specify a path
./configure --prefix=/some/folder
make -j4
make install
```
after the build, if you have not installed it system-wide (default path to
/usr/local/bin), add swig to your `$PATH` or set the `$SWIG` environment variable
pointing to the 'swig' binary. Both options are valid for libag Makefile.

### 2) Build libag
Once you have SWIG built and set, invoke make with
`make bindings`. Several temporary files will be generated in the
bindings/python directory, but the important ones are: `libag.py` and
`_libag.so`. They are what you should take with you to use libag in any Python
script :-).

Please note that this 'extension module' has been compiled for your processor
architecture and CPython version. If you want to use it in other environments,
this process must be done again.

#### Small note about Python versions:
Some operating systems use Python 2 as the default version (even though they
have both), others use Python 3, and others just version 3 but without an
'alias' for the 'python' command, but just 'python3'.

The libag Makefile uses the `python-config` command to find the system's default
Python include paths and populates the environment variable `$PY_INC` with that
information. If you want to use a version other than the default, consider
changing the environment variable to meet your expectations. If, for example,
a system (which has both versions) uses Python 2 by default, and you want a
build for v3, consider running: `export PY_INC=$(python3-config --includes)`
before invoking the Makefile.

It is also important to note that builds for Python 2 and 3 are not compatible
with each other, i.e., a `_libag.so` compiled for Python 2 only works on Python 2,
and vice versa.

## Usage
Since Python is not C, there are some slight differences in usage between the
two languages. The following sections cover it in detail.

### Structures and Classes
Thanks to SWIG, the relationship of structures and classes is transparent, which
means that for every structure in C, there is an equivalent class in Python.
Reading and writing values is as expected, as is passing parameters, which means:
```c
/* C. */
struct ag_config config;
memset(&config, 0, sizeof(struct ag_config));
config.search_binary_files = 1;
config.num_workers = 4;

ag_init_config(config)
```
is exactly equivalent to:
```python
# Python
config = ag_config()
config.search_binary_files = 1
config.num_workers = 4

ag_init_config(config)
```

### Functions
Functions that take no parameters and/or that take structs and return primitive
types have the expected behavior in both languages, which goes for: `ag_init()`,
`ag_init_config()`, `ag_finish()`, `ag_set_config()`, `ag_get_stats()`,
`ag_start_workers()`, `ag_stop_workers()` and `ag_free_result()`.

Those that differ are listed in the table below:
| Function                | C                                                                                                                                                                                                                    | Python equivalent                                                                                                                                                                          |
|-------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **ag_search**           | Parameters:<br>query (**char \***), npaths (**int**), target_paths (**char\*\***), nresults (**size_t\***)<br><br>Return:<br>On success: **struct ag_result \*\***, nresults (**size_t\***)<br>On error: **null**, 0 | Parameters:<br>query (**string**), target_paths (list of strings)<br><br>Return:<br>On success: tuple of (nresults (integer), tuple(**ag_result**))<br>On error: tuple of (0, **Py_None**) |
| **ag_free_all_results** | Parameters:<br>results (**struct ag_result \*\***), nresults (**size_t**)<br><br>Return: nothing                                                                                                                     | Parameters:<br>tuple(**ag_result**)<br><br>Return: nothing                                                                                                                                 |

Please note that although Python has a garbage collector, the memory allocated
in `ag_search()` _needs_ to be freed via `ag_free_result()` or
`ag_free_all_results()` (preferred).

---

Examples of how to use bindings can be found in
[bindings/python/examples](https://github.com/Theldus/libag/tree/master/bindings/python/examples).
