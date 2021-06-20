# Node.js bindings

## Introduction
Libag has a (very) experimental bindings support for Node.js

Unlike the
[Python bindings](https://github.com/Theldus/libag/tree/master/bindings/python),
unfortunately, the Node.js bindings could not be written using
[SWIG](http://www.swig.org/): seems like SWIG uses the v8 API directly, which is
always changing; therefore, in the latest SWIG version (v4.0.2), the generated code
does not work for the latest LTS version of Node (v14.17.1).

That said, the bindings for Node.js were implemented manually using the
[Node-API](https://nodejs.org/dist/latest-v14.x/docs/api/n-api.html), which is an
API implemented by Node.js itself that aims to remain stable between different
versions of Node.js. For any project aiming to implement addons to Node, the
Node-API seems to be the smarter/safer path to take.

## Building
The build process of the Node.js addon is as follows:

### 1) Download and configure Node.js
Download the latest version (or
[v14.17.1](https://nodejs.org/dist/v14.17.1/node-v14.17.1-linux-x64.tar.xz),
tested here) and set the environment variables to match your 'prefix':

Something like:
```bash
# Download Node.js
cd /path/to/nodejs
wget https://nodejs.org/dist/v14.17.1/node-v14.17.1-linux-x64.tar.xz
tar xf node*.tar.xz

# Configure environment variables and set to your ~/.bashrc too
export PATH=$PATH:$(readlink -f node-*/bin)
echo -e "# Node.js\nexport PATH=\$PATH:$(readlink -f node-*/bin)" >> ~/.bashrc

# Install cmake-js
npm install -g cmake-js
```
### 2) Build libag addon
Once Node.js and cmake-js were configured, there are several ways to generate the
addon, depending on how libag is located on your system:

#### a) Installed system-wide
If you already have the libag properly built and installed, navigate to the
`bindings/javascript` folder and invoke `cmake-js`, something like:
```bash
cd libag/bindings/javascript/
cmake-js
```
#### b) Installed in custom path
If you installed via Makefile or CMake in a PREFIX other than the default, invoke
`cmake-js` by providing the installation PATH, something like:
```bash
cd libag/bindings/javascript/
cmake-js --CDCMAKE_PREFIX_PATH=/your/libag/path/here
```
#### c) Not installed
Even if you don't have libag installed, you can still compile and use the module.
First, make sure libag is built properly, then invoke `cmake-js` as usual.

Note that in this case, the generated wrapper has the hardcoded path of the
compiled libag.so, so if this file changes location, the addon will not work:
```bash
cd libag/
make -j4
cd bindings/javascript/
cmake-js
```
## Usage
The differences between C and Javascript are covered below:

### Structures and Objects
All libag's C structures are mapped to javascript objects. It is important to note
that, unlike Python bindings, these objects are not classes and therefore do not
have 'constructors' in the strict sense of the word.

In this case, the C structures have a function of the same name in JS, and when
invoking them, a new object (wrapper) of the structure in C is created, which means:
```c
/* C. */
struct ag_config config;
memset(&config, 0, sizeof(struct ag_config));
config.search_binary_files = 1;
config.num_workers = 4;

ag_init_config(config)
```
is exactly equivalent to:
```javascript
/* Node.js. */
config = libag.ag_config();
config.search_binary_files = 1;
config.num_workers = 4;

libag.ag_init_config(config);
```

### Functions
Functions that take no parameters and/or that take structs and return primitive
types have the expected behavior in both languages, which goes for: `ag_init()`,
`ag_init_config()`, `ag_finish()`, `ag_set_config()`, `ag_get_stats()`,
`ag_start_workers()` and `ag_stop_workers()`.

Those that differ are listed in the table below:
| Function                | C                                                                                                                                                                                                                    | Javascript equivalent                                                                                                                                                                                  |
|-------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **ag_search**           | Parameters:<br>query (**char \***), npaths (**int**), target_paths (**char\*\***), nresults (**size_t\***)<br><br>Return:<br>On success: **struct ag_result \*\***, nresults (**size_t\***)<br>On error: **null**, 0 | Parameters:<br>query (**string**), target_paths (array of strings)<br><br>Return:<br>On success: pure JS object (not wrapper) containing (nresults, array of (**results**))<br>On error: undefined |

Unlike Python bindings, the return of `ag_search()` is a pure JS object, not a
wrapper. Thus, routines like `ag_free_result()` and `ag_free_all_results()`
unnecessary (in fact, they don't even exist in the binding).

---

Examples of how to use bindings can be found in
[bindings/javascript/examples](https://github.com/Theldus/libag/tree/master/bindings/javascript/examples).
