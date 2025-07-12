#include "hopscotch_ht_test_misc.h"

bool test_lookup_for_specific_key_value(
	hopscotch_hash_table_t *h,
	hash_function_f hash_function,
	size_t ht_size
) {
	hopscotch_hash_table_t* ht;
	bool is_local_ht = false;
	bool ret_val = false;
	uint32_t  number_of_elements;
	uint32_t inserted_els = 0;
	BENCHMARK_INIT;

	printf("[TEST %s] Started\n", __func__);
	if(h) {
		ht = h;
		number_of_elements = ANY_PERCENT(ht->capacity, 80);
	} else {
		ht = ht_create(ht_size);
		if(!ht) {
			printf("[TEST %s] Error: Unable to create hash table\n", __func__);
			return false;
		}
		is_local_ht = true;
		number_of_elements = ANY_PERCENT(ht_size, 80);
	}
	if(ht->size > 0) {
		printf("[TEST %s] Error: Hash table is not empty\n", __func__);
		return false;
	}

	test_data_t *pdata = allocate_test_data(number_of_elements);
	if(pdata == NULL) {
		printf("[TEST %s] Error: Unable to allocate test elements\n", __func__);
		return false;
	}
	printf("[TEST %s] Data\n", __func__);
	printf("[TEST %s] Hash table size    %ld\n", __func__, ht_size);
	printf("[TEST %s] Number of elements %d\n",  __func__, number_of_elements);
	printf("[TEST %s] Local hash table   %d\n",  __func__, is_local_ht);
	printf("[TEST %s] INSERT flow started... \n", __func__);

	BENCHMARK_START
	for(size_t i = 0; i < number_of_elements; i++) {
		if(ht_insert(ht, hash_function, pdata[i].key, pdata[i].value)) {
			pdata[i].inserted = true;
			inserted_els++;
		}
	}
	printf("[TEST %s] INSERT flow completed \n", __func__);
	printf("[TEST %s] Number of inserted elements %d\n", __func__, inserted_els);
	size_t idx_el_to_lookup = 0;
	for(idx_el_to_lookup  = number_of_elements / 2; 
		idx_el_to_lookup < number_of_elements; 
		idx_el_to_lookup++) {
		if(pdata[idx_el_to_lookup].inserted) break;
	}
	printf("[TEST %s] Random K/V selected ", __func__);
	PRINT_KEY_VALUE(pdata[idx_el_to_lookup].key, pdata[idx_el_to_lookup].value);
	uint8_t key[KEY_SIZE];
	uint8_t expected_value[VALUE_SIZE];
	uint8_t got_value[VALUE_SIZE];
	memcpy(key, pdata[idx_el_to_lookup].key, KEY_SIZE);
	memcpy(expected_value, pdata[idx_el_to_lookup].value, VALUE_SIZE);
	if(ht_contains_key(ht, hash_function, key, got_value)) {
		printf("[TEST %s] Contains value obtained\n", __func__);
	}
	BENCHMARK_END

	printf("[TEST %s] Obtained K/V ", __func__);
	PRINT_KEY_VALUE(key, got_value);
	if(memcmp(expected_value, got_value, KEY_SIZE) == 0) {
		printf("[TEST %s] Keys K/V match\n", __func__);
		ret_val = true;
	} else {
		printf("[TEST %s] Error: Keys K/V match error\n", __func__);
		ret_val = false;
	}

	if(is_local_ht) {
		free(ht);
		ht = NULL;
	}
	free_test_data(pdata, number_of_elements);
	
	if(ret_val) {
		BENCHMARK_MEASURE_THROUGHPUT(number_of_elements);
		printf("[TEST %s] PASSED successfully ", __func__);
		BENCHMARK_DATA_PRINT;
	}
	return ret_val;
}

bool test_insert_remove_elements(
	size_t number_of_elements,
	hash_function_f hash_function,
	bool ht_twice_size,
	bool validate_contains,
	bool need_to_remove
) {
	if(number_of_elements == 0) {
		printf("[TEST %s] Error: number of elements is 0\n", __func__);
		return false;
	}

	size_t capacity = number_of_elements;

	// Allocate twice time memory for table.
	if(ht_twice_size) {
		capacity = capacity * 2 ;
	}
	capacity = round_to_power_of_two(capacity);
	
	printf("[TEST %s] Stared\n", __func__);
	printf("[TEST %s] Table capacity : %ld\n", __func__, capacity);

	hopscotch_hash_table_t* ht = ht_create(capacity);
	if(ht == NULL) {
		printf("[TEST %s] Error: Unable to create hash table\n", __func__);
		return false;
	}

	// Prepare reference elements.
	number_of_elements = ANY_PERCENT(capacity, 80);
	test_data_t *pdata = allocate_test_data(number_of_elements);
	if(pdata == NULL) {
		ht_free(ht);
		printf("[TEST %s] Error: unable to allocate test keys and values\n", __func__);
		return false;
	}

	//--------------------------------------------------------------------------
	// INSERT.
	//--------------------------------------------------------------------------
	printf("[TEST %s] Number of elements to insert %ld\n", __func__, number_of_elements);
	int missing_inserted_els = 0;
	int inserted_els = 0;
	printf("[TEST %s] INSERT flow started... \n", __func__);
	for(size_t i = 0; i < number_of_elements; i++) {
		if(!ht_insert(ht, hash_function, pdata[i].key, pdata[i].value)) {
			missing_inserted_els++;
		} else {
			pdata[i].inserted = true;
			inserted_els++;
		}
	}
	printf("[TEST %s] Insert flow completed. Flow summary:\n", __func__);
	printf("[TEST %s] Missing  elements : %d\n", __func__, missing_inserted_els);
	printf("[TEST %s] Inserted elements : %d\n", __func__, inserted_els);
	ht_print_stats(ht);
	// More detailed data (in case if there are not so many elements)
	if(inserted_els <= 0xFF)
		ht_print_debug(ht);

	//--------------------------------------------------------------------------
	// VALIDATE CONTAINS DATA.
	//--------------------------------------------------------------------------
	if(validate_contains) {
		printf("[TEST %s] VALIDATE keys flow started...\n", __func__);
		int missing = 0;
		for(size_t i = 0; i < number_of_elements; i++) {
			if(!pdata[i].inserted) {
				continue;
			}
			if(!ht_contains_key(ht, hash_function, pdata[i].key, NULL)) {
				missing++;
			}
		}
		printf("[TEST %s] Validate keys flow completed. Flow summary:\n", __func__);
		printf("[TEST %s] Missing validated elements: %d\n", __func__, missing);
	}

	//--------------------------------------------------------------------------
	// REMOVE ALL DATA.
	//--------------------------------------------------------------------------
	if(need_to_remove) {
		printf("[TEST %s] REMOVE keys flow started...\n", __func__);
		int missing_removes = 0;
		for(size_t i = 0; i < number_of_elements; i++) {
			if(!pdata[i].inserted) {
				continue;
			}
			if(!ht_remove_key(ht, hash_function, pdata[i].key)) {
				missing_removes++;
			}
		}
		printf("[TEST %s] Remove keys flow completed. Flow summary:\n", __func__);
		printf("[TEST %s] Missing elements: %d\n", __func__, missing_removes);
		printf("[TEST %s] Printint hash table info\n", __func__);
		ht_print_stats(ht);
		ht_print_debug(ht);
	}
	free_test_data(pdata, number_of_elements);
	ht_free(ht);
	pdata = NULL;
	printf("[TEST %s] PASSED successfully\n", __func__);
	return true;
}

bool test_relocation_and_max_relocation_value() {
	size_t table_size = round_to_power_of_two(0xFF);

	// Only the single elements will be missed.
	const uint8_t els_in_exceeded_relocation_region = 1;
	uint32_t number_of_elements = HOP_RANGE * MAX_RELOCATION_FACTOR + \
									els_in_exceeded_relocation_region; 
	uint8_t els_in_exceeded_relocation = 0;

	printf("[TEST %s] Started\n", __func__);
	printf("[TEST %s] Table size %ld\n", __func__, table_size);
	hopscotch_hash_table_t* ht = ht_create(table_size);
	if(ht == NULL) {
		printf("[TEST %s] FAILED: Unable to create hash table\n", __func__);
		return false;
	}
	test_data_t *pdata = allocate_test_data(number_of_elements);
	if(pdata == NULL) {
		printf("[TEST %s] FAILED: Unable to allocate test keys and values\n", __func__);
		return false;
	}
	printf("[TEST %s] Number of elements: %d\n", __func__, number_of_elements);
	for(size_t i = 0; i < number_of_elements; i += 10) {
		printf("[TEST %s] ", __func__);
		PRINT_KEY_VALUE(pdata[i].key, pdata[i].value);
		if(i != number_of_elements - 1)
			printf("[TEST %s] ... \n", __func__);
	}
	for(size_t i = 0; i < number_of_elements; i ++) {
		if(!ht_insert(ht, dummy_set_1_hash, pdata[i].key, pdata[i].value)) {
			els_in_exceeded_relocation++;
			printf("[TEST %s] INFO MSG: Unable to relocate ", __func__);
			PRINT_KEY_VALUE(pdata[i].key, pdata[i].value);
		}
	}
	if(els_in_exceeded_relocation_region != els_in_exceeded_relocation) {
		printf("[TEST %s] Failed number of elements are not matched\n", __func__);
		return false;
	}

	uint8_t idx_to_fetch = number_of_elements - 0x10;
	uint8_t got_value[VALUE_SIZE];
	printf("[TEST %s] Index to fetch %d\n", __func__, idx_to_fetch);
	if(!ht_contains_key(ht, dummy_set_1_hash, pdata[idx_to_fetch].key, got_value)) {
		printf("[TEST %s] Failed to fetch element ", __func__);
		PRINT_KEY_VALUE(pdata[idx_to_fetch].key, pdata[idx_to_fetch].value);
		return false;
	}
	if(memcmp(pdata[idx_to_fetch].value, got_value, KEY_SIZE) == 0) {
		printf("[TEST %s] Keys K/V match\n", __func__);
	} else {
		printf("[TEST %s] FAILED Keys K/V match error\n", __func__);
		return false;
	}

	ht_print_debug(ht);
	printf("[TEST %s] PASSED successfully\n", __func__);

	ht_free(ht);
	return true;
}
