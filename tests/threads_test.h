#ifndef THREADS_TEST_H
#define THREADS_TEST_H

#include "hopscotch_ht_test_misc.h"

//------------------------------------------------------------------------------
// Insert thread data.
//------------------------------------------------------------------------------
typedef struct {
	hopscotch_hash_table_t *ht;
	hash_function_f hash_function;
	test_data_t *pdata;
	atomic_char **progress_stages;
	size_t test_data_size;
	size_t number_of_threads;
	int thread_id;
	int keys_to_insert;
	atomic_int *keys_inserted;
	atomic_int *keys_validated;
	atomic_int *keys_removed;
	_Atomic (ht_benchmark_data_t *) benchmark_data;
} ht_thread_insert_data_t;
int thread_insert_worker(void *arg);

//------------------------------------------------------------------------------
// Print progress thread data.
//------------------------------------------------------------------------------
typedef struct {
	atomic_char **progress_stages;
	size_t number_of_threads;
	_Atomic (ht_benchmark_data_t *) benchmark_data;
} ht_thread_print_progress_data_t;

typedef enum {
	PROGRESS_STAGE_INSERT = 0,
	PROGRESS_STAGE_CONTAINS,
	PROGRESS_STAGE_REMOVE,
	PROGRESS_STAGE_TOTAL
} PROGRESS_STAGE;

void update_progress(atomic_char **progress_stages, int thread_id, int stage);
int thread_print_progress_worker(void *arg);

//------------------------------------------------------------------------------
// Memory management structure not to forget remove all data.
//------------------------------------------------------------------------------
typedef struct _concurrent_inserts_mm_data {
	size_t number_of_threads;
	size_t number_of_elements;
	thrd_t *threads_insert_worker;
	ht_thread_insert_data_t *thread_insert_worker_data;
	ht_benchmark_data_t *thread_benchmark_data;
	test_data_t *pdata;
	_Atomic(char) **progress_stages;
} concurrent_inserts_mm_data;

void free_concurrent_inserts_mm_data(concurrent_inserts_mm_data *data);

#define MM_DATA_INIT \
    concurrent_inserts_mm_data _data __attribute__((unused)) = {0}

#define MM_DATA_WRITE(field) _data.field = field

#define MM_DATA_FREE free_concurrent_inserts_mm_data(&_data)

#endif
