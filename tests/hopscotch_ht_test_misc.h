#ifndef HOPSCOTCH_HT_TEST_H
#define HOPSCOTCH_HT_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#include <threads.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "hopscotch_ht.h"

//------------------------------------------------------------------------------
// Benchmark specific struct and macros.
//------------------------------------------------------------------------------
#define BENCHMARK_INIT \
	double _start_time = 0, _end_time   = 0; \
	double _elapsed    = 0, _throughput = 0;

#define BENCHMARK_START \
	do { \
		_start_time = get_current_time(); \
	} while(0);

#define BENCHMARK_END \
	do { \
		_end_time = get_current_time(); \
		_elapsed = _end_time - _start_time; \
	} while(0);

#define BENCHMARK_MEASURE_THROUGHPUT(n) \
	do { \
		_throughput = n / _elapsed; \
	} while(0);

#define BENCHMARK_DATA_PRINT \
	do { \
		if(_throughput != 0) \
			printf("Time: %.4f sec, throughput: %.2f ops/sec\n", \
				_elapsed, _throughput); \
		else \
			printf("Time: %.4f sec\n", _elapsed); \
	} while(0);

#define BENCHMARK_GET_ELAPSED _elapsed

#define BENCHMARK_GET_THROUGHPUT _throughput

typedef struct {
	_Atomic(double) elapsed_time;
	_Atomic(double) throughput_value;
} ht_benchmark_data_t;
double get_current_time();

//------------------------------------------------------------------------------
// Test data generation struct and functions.
//------------------------------------------------------------------------------
typedef struct {
	uint8_t *key;
	uint8_t *value;
	bool inserted;
} test_data_t;
test_data_t *allocate_test_data(size_t );

int generate_random_pair(uint8_t *, uint8_t *);
test_data_t *allocate_test_data(size_t );
void free_test_data(test_data_t *, size_t );

//------------------------------------------------------------------------------
// Other functions and macros.
//------------------------------------------------------------------------------
#define ANY_PERCENT(size, any) ((size_t)((size) * any / 100))

size_t round_to_power_of_two(size_t v);
uint32_t round_to_power_of_two_32(uint32_t v);
uint64_t round_to_power_of_two_64(uint64_t v);

#endif // HOPSCOTCH_HT_TEST_H
