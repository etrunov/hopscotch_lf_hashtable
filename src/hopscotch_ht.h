#ifndef HOPSCOTCH_HT_H
#define HOPSCOTCH_HT_H

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

//------------------------------------------------------------------------------
// Hash table related functions and defines.
//------------------------------------------------------------------------------
#define KEY_SIZE (64)
#define VALUE_SIZE (128)
#define HOP_RANGE (32)
#define MAX_RELOCATION_FACTOR (5)
#define HASH_HOP_INFO_OFFSET (32)
#define HOP_INFO_MASK (0xFFFFFFFF)
#define HASH_MASK (0xFFFFFFFF00000000)

#define INDEX(hash, mask) ((hash) & (mask))
#define PRINT_KEY_VALUE(_k, _v) \
	do { \
		if(_k != NULL) { \
			printf("K: "); \
			for(size_t _i = 0; _i < 4; _i++) { \
				printf("%02X", _k[_i]); \
			} \
			printf("...   "); \
		} \
		if(_k != NULL) { \
			printf("V: "); \
			for(size_t _i = 0; _i < 4; _i++) { \
				printf("%02X", _v[_i]); \
			} \
			printf("...\n"); \
		} \
	} while(0);



/*
hop_info type diagram.
Lower bits for hop, upper for hash
+-----------+------------+
| 63 ... 32 | 31 3 2 1 0 |
|-----------|------------|
|   Hash    |  Hop bits  |
+-----------+------------+
*/
typedef struct {
	uint8_t value[VALUE_SIZE];
	uint8_t key[KEY_SIZE];
	atomic_uint_fast64_t hop_info; // Lower bits for hop, upper for hash
} hash_node_t;

// %32 size
typedef struct {
	hash_node_t* nodes;
	_Atomic size_t size;
	size_t capacity;
	size_t mask;
} hopscotch_hash_table_t;

//------------------------------------------------------------------------------
// Hash functions related block.
//------------------------------------------------------------------------------
typedef uint32_t (*hash_function_f)(const uint8_t *);

// MurmurHash3 custom.
uint32_t murmur_custom_hash(const uint8_t* );

// Jenkins hash function.
uint32_t jenkins_one_at_a_time_hash(const uint8_t *);

// Dummy hash function to test collisions.
uint32_t dummy_set_1_hash(const uint8_t *);

//------------------------------------------------------------------------------
// Hash table related functions / API.
//------------------------------------------------------------------------------
void ht_print_debug(const hopscotch_hash_table_t * const ht);
void ht_print_stats(const hopscotch_hash_table_t * const ht);
void ht_zero(hopscotch_hash_table_t *ht);
hopscotch_hash_table_t *ht_create(size_t capacity);
void ht_free(hopscotch_hash_table_t *ht);
bool ht_insert(
	hopscotch_hash_table_t* ht,
	hash_function_f hash_key,
	const uint8_t *key,
	const uint8_t *value
);
bool ht_remove_key(
	hopscotch_hash_table_t *ht,
	hash_function_f hash_function,
	const uint8_t *key
);

bool ht_contains_key(
	const hopscotch_hash_table_t *ht,
	hash_function_f hash_function,
	const uint8_t *key,
	uint8_t *out_value
);
#endif /* HOPSCOTCH_HT_H */
