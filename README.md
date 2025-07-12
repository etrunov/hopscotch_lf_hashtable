
# Task description

In C language, implement a hash table ```<key=64bytes, node=128bytes>```
with flat addressing (this means memory for the entire table, including memory
for buckets and nodes, should be allocated in a single, continuous buffer).
A hash table implementation cannot use mutex or spinlocks locking primitives.
The collision resolution scheme must be fast. Implement a consistency check
when adding 100,000,000 random nodes with random keys.
Add multithreading of 32 threads, pre-fill the hash map by 3/4 (load factor)
of the occupied memory, but not less than 1,000,000 nodes, in each thread
endlessly add a random node with a random key, check for the
presence of the added node and delete.

# Hash table
The hash table in this project is based on the Hopscotch Hashing algorithm, a collision-resolution scheme that combines open addressing with linear probing and neighborhood constraints for efficient lookups.

[1] [Hopscotch Hashing - General Algorithm (Wikipedia)](https://en.wikipedia.org/wiki/Hopscotch_hashing)

[2] [Hopscotch Hashing - Original Paper (Web Archive)](https://web.archive.org/web/20221220235913/http://mcg.cs.tau.ac.il/papers/disc2008-hopscotch.pdf)

[3] [Lock-Free Hopscotch Hashing (Maynooth University)](https://mural.maynoothuniversity.ie/id/eprint/15097/1/BP_lock-free.pdf) 

- Note: This reference provides pseudo-code.

## Hash Functions
The implementation supports two hash functions for key generation:

[1] MurmurHash3

- Custom [MurmurHash3 Algorithm (Wikipedia)](https://en.wikipedia.org/wiki/MurmurHash)

- A non-cryptographic hash function optimized for speed and distribution.

[2] Jenkins Hash Function

 - [Jenkins hash function (Wikipedia)](https://en.wikipedia.org/wiki/Jenkins_hash_function)

 - Designed for robustness and minimal collisions in lookup operations.

Note: The flow supports pluggable hash functions adhering to the following prototype:
```typedef uint32_t (*hash_function_f)(const uint8_t *);```

# Project Structure

The project follows a standardized directory structure to maintain clarity and separation of concerns.

## Source Code (`src/`)
### Description
Contains the primary implementation of the Hopscotch Hashing algorithm and associated data structures.

### Contents
- Core hash table implementation files:
  - `hashtable.c` - Main hash table operations.
  - `hashtable.h` - Public interface and definitions.

## Test Suite (`tests/`)
### Description

Contains number of tests to validate the hash table algorithm.

# Hash Table API

All public functions and macros are defined in the `hopscotch_ht.h` header file, which must be included in any project using this implementation.

## Function Reference

| Function/Macro        | Parameters                        | Description                                                                 |
|-----------------------|-----------------------------------|-----------------------------------------------------------------------------|
| `ht_create`           | `size`                            | Creates and initializes a new hash table with the specified capacity.       |
| `ht_free`             | `hash_t *`                        | Deallocates all resources associated with the hash table.                   |
| `ht_zero`             | `hash_t *`                        | Resets all entries in the hash table while maintaining its capacity.        |
| `ht_insert`           | `hash_t *, hash_f, k, v`          | Inserts a key-value pair into the table (returns false on collision/full).  |
| `ht_remove_key`       | `hash_t *, hash_f, k`             | Removes the specified key and its associated value from the table.          |
| `ht_contains_key`     | `hash_t *, hash_f, k, val *out`   | Checks for key existence (optional: outputs value via pointer if non-NULL). |
| `ht_print_debug`      | `const hash_t *`                  | Prints complete table contents for debugging purposes.                      |
| `ht_print_stats`      | `const hash_t *`                  | Outputs operational statistics (load factor etc.).                          |
| `PRINT_KEY_VALUE`     | `k,  v`                           | Macro for printing key-value pairs.                                         |

### Type Aliases
- `hopscotch_hash_table_t` → `hash_t`.
- `hash_function_f` → `hash_f`.
- Key/Value type: `uint8_t*` (pointer to byte array).

## Usage Notes

1. **Memory Management**:
   - `ht_create` allocates memory that must be freed with `ht_free`.
   - Keys and values are copied by value (caller retains ownership). There is the function in `hopscotch_ht_test_misc.h` to work with random keys and values `allocate_test_data()`  and `free_test_data()`.

2. **Error Handling**:
   - Functions returning bool indicate success (true) or failure (false).
   - NULL pointer parameters are considered invalid input (false).

3. **Thread Safety**:
   - Implementation uses atomic primitives for thread-safe operations.

# Testing Strategy

- All test implementations must reside in the `tests/` directory.

## Test Interface
- Test function prototypes are declared in `hopscotch_ht_test_iface.h`
- This interface file is included by:
  - `hopscotch_ht_main.c` (the main app runner).
  - Individual test implementations must include `hopscotch_ht_test_misc.h` with all test-related functions.

## Supporting Utilities
The `hopscotch_ht_test_misc.h` header provides:
- Benchmarking macros for performance measurement.
- Helper functions for randomized test data generation.

# Build and Execution Instructions
The current implementation is exclusively compatible with **Linux-based systems**. Windows has not been tested.

## Requirements
- **CMake** ≥ 3.10.
- **C11-compatible compiler**.
- **Bash** shell environment.

## Building the Project

### Standard Build Procedure
1. Navigate to the project root directory.
2. Execute the build script:
   ```bash
   ./build.sh
3. Run `./hopscotch_ht_app`

### Standard clen Procedure
1. Navigate to the project root directory.
2. Execute the build script:
   ```bash
   ./build.sh clean
