#ifndef HOPSCOTCH_HT_TEST_IFACE_H
#define HOPSCOTCH_HT_TEST_IFACE_H

#include "hopscotch_ht_test_misc.h"

/*
The test performs Insert, Contains, and Remove (Erase) operations on the 
hash table.

Parameters:
number_of_elements – The desired number of elements to be processed.
hash_function - hash function to be tested (available in hopscotch_ht.h).
ht_twice_size - should hash table be twice size of desired number of elements.
number_of_threads - Total number of threads for the hash table operations.

Note:
The actual number of elements will be adjusted using the round_to_power_of_two function.
The final number of elements will be further limited to 80% of the rounded value.
*/
bool test_run_concurrent(
	size_t number_of_elements,
	hash_function_f hash_function,
	bool ht_twice_size,
	size_t number_of_threads
);

/*
The test allocates data and fetch/search/lookup for the specific key/value in
the hash table. In case if h is null, the test will create ht_size hash table.
*/
bool test_lookup_for_specific_key_value(
	hopscotch_hash_table_t *h,
	hash_function_f hash_function,
	size_t ht_size
);

/*
Basic test to perform Hash table operations:
- Insert - mandatory;
- Contains (optional, if validate_contains is defined);
- Remove (optional, if need_to_remove is defined);
Option ht_twice_size will create a hash table twice size number of elements.
*/
bool test_insert_remove_elements(
	size_t number_of_elements,
	hash_function_f hash_function,
	bool ht_twice_size,
	bool validate_contains,
	bool need_to_remove
);

/*
The test checks that relocation region cannot be more than 
HOP_RANGE(32) * MAX_RELOCATION_FACTOR(5) - based on the define.
The only one element must be out of scope els_in_exceeded_relocation_region = 1
The test does:
1. 161 random key values inserts.
2. Check that the 161st key is not inserted.
3. Fetch 161 - 10 element from the hash table.
4. Print hash table that must be fancy rendered from neighborhood point of view
   based on HOP_RANGE value (the fragment below).
Hopscotch Hash Table (Capacity: 256, Size: 160)
-----------------------------------------------------------------------------------------
IDX   Hom->Cur Hash     Hop bits     Key....  Val....  Neighborhood (32)
-----------------------------------------------------------------------------------------
[001] 001->001 00000001 00.00.00.01  26B6...  0A50...  [x...............................]
[002] 001->002 00000001 00.00.00.02  F068...  641A...  [.x..............................]
[003] 001->003 00000001 00.00.00.04  DD58...  8687...  [..x.............................]
[004] 001->004 00000001 00.00.00.08  6E13...  9CAF...  [...x............................]
[005] 001->005 00000001 00.00.00.10  FC0C...  CB60...  [....x...........................]
[006] 001->006 00000001 00.00.00.20  79CC...  3016...  [.....x..........................]
[007] 001->007 00000001 00.00.00.40  B0C4...  AE10...  [......x.........................]
[008] 001->008 00000001 00.00.00.80  46DC...  7872...  [.......x........................]
[009] 001->009 00000001 00.00.01.00  6C82...  50A0...  [........x.......................]
[010] 001->010 00000001 00.00.02.00  2771...  82BA...  [.........x......................]
[011] 001->011 00000001 00.00.04.00  BE58...  1581...  [..........x.....................]
[012] 001->012 00000001 00.00.08.00  0141...  33F6...  [...........x....................]
[013] 001->013 00000001 00.00.10.00  585D...  A981...  [............x...................]
[014] 001->014 00000001 00.00.20.00  3972...  940D...  [.............x..................]
[015] 001->015 00000001 00.00.40.00  1F71...  6053...  [..............x.................]
[016] 001->016 00000001 00.00.80.00  3C57...  BFD1...  [...............x................]
[017] 001->017 00000001 00.01.00.00  49A6...  7FC6...  [................x...............]
[018] 001->018 00000001 00.02.00.00  9185...  A528...  [.................x..............]
[019] 001->019 00000001 00.04.00.00  D26C...  5671...  [..................x.............]
[020] 001->020 00000001 00.08.00.00  96C9...  A579...  [...................x............]
[021] 001->021 00000001 00.10.00.00  FEE6...  C1D5...  [....................x...........]
[022] 001->022 00000001 00.20.00.00  6074...  A074...  [.....................x..........]
[023] 001->023 00000001 00.40.00.00  00E0...  ABAF...  [......................x.........]
[024] 001->024 00000001 00.80.00.00  B781...  3451...  [.......................x........]
[025] 001->025 00000001 01.00.00.00  E1E1...  9BC1...  [........................x.......]
[026] 001->026 00000001 02.00.00.00  D7C6...  D907...  [.........................x......]
[027] 001->027 00000001 04.00.00.00  2C51...  C180...  [..........................x.....]
[028] 001->028 00000001 08.00.00.00  10C3...  6B65...  [...........................x....]
[029] 001->029 00000001 10.00.00.00  B945...  CCF3...  [............................x...]
[030] 001->030 00000001 20.00.00.00  68D0...  4578...  [.............................x..]
[031] 001->031 00000001 40.00.00.00  FFD0...  268F...  [..............................x.]
[032] 001->032 00000001 80.00.00.00  0FE5...  D9DC...  [...............................x]
[033] 001->033 00000001 00.00.00.01  0C77...  B985...  [x...............................]
[034] 001->034 00000001 00.00.00.02  082C...  7066...  [.x..............................]
[035] 001->035 00000001 00.00.00.04  980C...  7F07...  [..x.............................]
*/
bool test_relocation_and_max_relocation_value();

#endif // HOPSCOTCH_HT_TEST_IFACE_H
