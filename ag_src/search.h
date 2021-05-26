#ifndef SEARCH_H
#define SEARCH_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pcre.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "decompress.h"
#include "ignore.h"
#include "log.h"
#include "options.h"
#include "print.h"
#include "uthash.h"
#include "util.h"

extern size_t alpha_skip_lookup[256];
extern size_t *find_skip_lookup;
extern uint8_t h_table[H_SIZE] __attribute__((aligned(64)));

struct work_queue_t {
    char *path;
    struct work_queue_t *next;
};
typedef struct work_queue_t work_queue_t;

extern work_queue_t *work_queue;
extern work_queue_t *work_queue_tail;
extern int done_adding_files;
extern int stop_workers;
extern pthread_cond_t files_ready;
extern pthread_mutex_t stats_mtx;
extern pthread_mutex_t work_queue_mtx;

extern pthread_barrier_t worker_done;
extern pthread_barrier_t results_done;

/* For symlink loop detection */
#define SYMLOOP_ERROR (-1)
#define SYMLOOP_OK (0)
#define SYMLOOP_LOOP (1)

typedef struct {
    dev_t dev;
    ino_t ino;
} dirkey_t;

typedef struct {
    dirkey_t key;
    UT_hash_handle hh;
} symdir_t;

extern symdir_t *symhash;

void search_buf(int worker_id, const char *buf, const size_t buf_len,
                const char *dir_full_path);
void search_stream(int worker_id, FILE *stream, const char *path);
void search_file(int worker_id, const char *file_full_path);

void *search_file_worker(void *i);

void search_dir(ignores *ig, const char *base_path, const char *path, const int depth, dev_t original_dev);

/* libag 'private' routines and variables. */
extern int add_local_result(int worker_id, const char *file,
    const match_t matches[], const size_t matches_len,
    const char *buf);

extern int init_local_results(int worker_id);
extern int has_ag_init;

#endif
