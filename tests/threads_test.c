#include "threads_test.h"

int thread_insert_worker(void *arg) {
	if(arg == NULL) {
		printf("Error: Unable to process args. Args are empty\n");
		return 1;
	}

	ht_thread_insert_data_t* data = (ht_thread_insert_data_t*)arg;
	
	// Calculate base and remaining elements.
	int base_elems = data->test_data_size / data->number_of_threads;
	int remaining_elems = data->test_data_size % data->number_of_threads;

	// Calculate start index with remainder distribution.
	int start_idx = data->thread_id * base_elems +
		(data->thread_id < remaining_elems ? data->thread_id : remaining_elems);

	// Calculate actual elements for this thread.
	int elems_for_this_thread =
		base_elems + (data->thread_id < remaining_elems ? 1 : 0);
	size_t end_idx = start_idx + elems_for_this_thread;
	
	// Ensure no exceed test_data_size.
	end_idx = end_idx > data->test_data_size ? data->test_data_size : end_idx;

	BENCHMARK_INIT;
	BENCHMARK_START;
	//--------------------------------------------------------------------------
	// INSERT.
	//--------------------------------------------------------------------------
	for(size_t i = start_idx; i < end_idx; i++) {
		if(ht_insert(
			data->ht,
			data->hash_function,
			data->pdata[i].key,
			data->pdata[i].value
		)) {
			atomic_fetch_add(data->keys_inserted, 1);
			data->pdata[i].inserted = true;
		}
	}
	update_progress(data->progress_stages, data->thread_id, PROGRESS_STAGE_INSERT);

	//--------------------------------------------------------------------------
	// VALIDATE CONTAINS DATA.
	//--------------------------------------------------------------------------
	for(size_t i = start_idx; i < end_idx; i++) {
		if(!data->pdata[i].inserted) continue;
		if(ht_contains_key(data->ht, data->hash_function,
			data->pdata[i].key, NULL)) {
			atomic_fetch_add(data->keys_validated, 1);
		}
	}
	update_progress(data->progress_stages, data->thread_id, PROGRESS_STAGE_CONTAINS);

	//--------------------------------------------------------------------------
	// REMOVE ALL DATA.
	//--------------------------------------------------------------------------
	for(size_t i = start_idx; i < end_idx; i++) {
		if(!data->pdata[i].inserted) continue;
		if(ht_remove_key(data->ht, data->hash_function, data->pdata[i].key)) {
			atomic_fetch_add(data->keys_removed, 1);
		}
	}
	update_progress(data->progress_stages, data->thread_id, PROGRESS_STAGE_REMOVE);
	BENCHMARK_END;
	BENCHMARK_MEASURE_THROUGHPUT(data->keys_to_insert);

	double elapsed_time = BENCHMARK_GET_ELAPSED;
	double throughput_value = BENCHMARK_GET_THROUGHPUT;
	atomic_store_explicit(
		&data->benchmark_data[data->thread_id].elapsed_time,
		elapsed_time,
		memory_order_release
	);
	atomic_store_explicit(
		&data->benchmark_data[data->thread_id].throughput_value,
		throughput_value,
		memory_order_release
	);
	return 0;
}

void update_progress(atomic_char **progress_stages, int thread_id, int stage) {
	atomic_store(&progress_stages[thread_id][stage], "ICR"[stage]);
}

int thread_print_progress_worker(void *arg) {
	if(arg == NULL) {
		printf("Error: Unable to process args thread_print_progress_worker. \
			Args are empty\n");
		return 1;
	}
	ht_thread_print_progress_data_t* data = (ht_thread_print_progress_data_t *)arg;
	atomic_char **progress_stages = data->progress_stages;
	size_t number_of_threads = data->number_of_threads;
	size_t complete_or_not = 0;

	printf("\033[H\033[J"); // Clear screen
	while(1) {
		printf("\033[H\033[J");
		printf("Performing operations...\n");
		for(size_t i = 0; i < number_of_threads; i++) {
			char i_stage = atomic_load(&progress_stages[i][PROGRESS_STAGE_INSERT]);
			char c_stage = atomic_load(&progress_stages[i][PROGRESS_STAGE_CONTAINS]);
			char r_stage = atomic_load(&progress_stages[i][PROGRESS_STAGE_REMOVE]);
			if(r_stage != ' ') complete_or_not++;
			printf("Thread %2ld: [%c %c %c]\n", i, i_stage, c_stage, r_stage);
		}
		if(complete_or_not != number_of_threads) {
			complete_or_not = 0;
		} else {
			printf("\033[H\033[J");
			printf("All tests has completed their flows.\n");
			for(size_t i = 0; i < number_of_threads; i++) {
				printf("Thread %2ld: [I C R]  %.4f sec %.2f ops/sec\n",
					i,
					atomic_load(&data->benchmark_data[i].elapsed_time),
					atomic_load(&data->benchmark_data[i].throughput_value)
				);
			}
			break;
		};
		usleep(10000); // 100ms delay
	}
	return 0;
}

void free_concurrent_inserts_mm_data(concurrent_inserts_mm_data *data) {
	if(!data) return;

	if(data->progress_stages) {
		for(size_t i = 0; i < data->number_of_threads; i++) {
			free(data->progress_stages[i]);
		}
		free(data->progress_stages);
	}

	if(data->threads_insert_worker)
		free(data->threads_insert_worker);

	if(data->thread_benchmark_data)
		free(data->thread_benchmark_data);

	if(data->pdata) {
		free_test_data(data->pdata, data->number_of_elements);
	}

	if(data->thread_insert_worker_data)
		free(data->thread_insert_worker_data);

	// Clear all pointers.
	data->threads_insert_worker = NULL;
	data->thread_insert_worker_data = NULL;
	data->pdata = NULL;
	data->progress_stages = NULL;
	data->thread_benchmark_data = NULL;
	data->number_of_elements = 0;
	data->number_of_threads = 0;
}

bool test_run_concurrent(
	size_t number_of_elements,
	hash_function_f hash_function,
	bool ht_twice_size,
	size_t number_of_threads
) {
	size_t capacity = number_of_elements;

	// Allocate twice time memory for table.
	if(ht_twice_size) {
		capacity = capacity * 2 ;
	}
	capacity = round_to_power_of_two(capacity);
	printf("[TEST %s] Started...\n", __func__);
	printf("[TEST %s] Table capacity : %ld\n", __func__, capacity);

	//--------------------------------------------------------------------------
	// Memory allocation.
	//--------------------------------------------------------------------------
	hopscotch_hash_table_t *ht = ht_create(capacity);
	if(ht == NULL) {
		printf("[TEST %s] Error: Unable to create Hash table\n", __func__);
		return false;
	}

	MM_DATA_INIT;
	MM_DATA_WRITE(number_of_threads);

	// Prepare reference elements.
	number_of_elements = ANY_PERCENT(capacity, 80);
	MM_DATA_WRITE(number_of_elements);

	thrd_t *threads_insert_worker  = malloc(sizeof(thrd_t) * number_of_threads);
	if(!threads_insert_worker) {
		printf("[TEST %s] Error: Unable to create threads_insert_worker\n", __func__);
		MM_DATA_FREE;
		ht_free(ht);
		return false;
	}
	memset(threads_insert_worker, 0, sizeof(thrd_t) * number_of_threads);
	MM_DATA_WRITE(threads_insert_worker);
	
	ht_thread_insert_data_t *thread_insert_worker_data = malloc(
		sizeof(ht_thread_insert_data_t) * number_of_threads);
	if(!thread_insert_worker_data) {
		printf("[TEST %s] Error: Unable to create thread_insert_worker_data\n", __func__);
		MM_DATA_FREE;
		ht_free(ht);
		return false;
	}
	MM_DATA_WRITE(thread_insert_worker_data);
	
	// Prepare data for print progress thread.
	thrd_t thread_print_progresst_worker;
	ht_thread_print_progress_data_t thread_print_progress_worker_data;
	ht_benchmark_data_t *thread_benchmark_data = malloc(
		sizeof(ht_benchmark_data_t) * number_of_threads);
	if(!thread_benchmark_data) {
		printf("[TEST %s] Error: Unable to create thread_benchmark_data\n", __func__);
		MM_DATA_FREE;
		ht_free(ht);
		return false;
	}
	MM_DATA_WRITE(thread_benchmark_data);

	atomic_int keys_inserted = 0;
	atomic_int keys_validated = 0;
	atomic_int keys_removed = 0;
	int keys_per_thread = number_of_elements / number_of_threads;

	test_data_t *pdata = allocate_test_data(number_of_elements);
	if(!pdata) {
		printf("[TEST %s] Error: Unable to allocate test keys and values\n", __func__);
		MM_DATA_FREE;
		ht_free(ht);
		return false;
	}
	MM_DATA_WRITE(pdata);

	// Create progress data
	// I - Insert; C - Contains; R - Remove;
	_Atomic(char) **progress_stages; // Pointer to 'I', 'C', 'R'
	progress_stages = malloc(sizeof(char *) * number_of_threads);
	for(size_t i = 0; i < number_of_threads; i++) {
		progress_stages[i] = malloc(sizeof(char) * PROGRESS_STAGE_TOTAL);
		if(!progress_stages[i]) {
			printf("[TEST %s] Error: Unable to create progress_stage %ld\n", __func__, i);
			MM_DATA_FREE;
			ht_free(ht);
			return false;
		}
		for(int j = 0; j < PROGRESS_STAGE_TOTAL; j++)
			atomic_init(&progress_stages[i][j], ' ');
	}
	MM_DATA_WRITE(progress_stages);


	printf("[TEST %s] Total number of threads            : %ld\n", __func__, number_of_threads);
	printf("[TEST %s] otal number of elements           : %ld\n", __func__, number_of_elements);
	printf("[TEST %s] Number of els to insert per thread : %d\n", __func__, keys_per_thread);
	sleep(1); // Wait for a user to read data.

	//--------------------------------------------------------------------------
	// Initialize thread_print_progress_worker data.
	//--------------------------------------------------------------------------
	thread_print_progress_worker_data = (ht_thread_print_progress_data_t){
		.progress_stages = progress_stages,
		.number_of_threads = number_of_threads,
		.benchmark_data = thread_benchmark_data
	};

	// Initialize thread_insert_worker data.
	for(size_t i = 0; i < number_of_threads; i++) {
		thread_insert_worker_data[i] = (ht_thread_insert_data_t){
			.ht = ht,
			.hash_function = hash_function,
			.pdata = pdata,
			.progress_stages = progress_stages,
			.test_data_size = number_of_elements,
			.number_of_threads = number_of_threads,
			.thread_id = i,
			.keys_to_insert = keys_per_thread,
			.keys_inserted = &keys_inserted,
			.keys_validated = &keys_validated,
			.keys_removed = &keys_removed,
			.benchmark_data = thread_benchmark_data
		};
		// Last thread gets remaining keys.
		if(i == number_of_threads - 1) {
			thread_insert_worker_data[i].keys_to_insert = number_of_elements -
				(keys_per_thread * (number_of_threads - 1));
		}
	}

	//--------------------------------------------------------------------------
	// Creating threads.
	//--------------------------------------------------------------------------
	if(thrd_create(&thread_print_progresst_worker, thread_print_progress_worker, 
			&thread_print_progress_worker_data) != thrd_success) {
		printf("[TEST %s] Error: Failed to create print progress thread\n", __func__);
		MM_DATA_FREE;
		ht_free(ht);
		return false;
	}

	//--------------------------------------------------------------------------
	// Create thread_insert_worker.
	//--------------------------------------------------------------------------
	for(size_t i = 0; i < number_of_threads; i++) {
		if(thrd_create(&threads_insert_worker[i], thread_insert_worker, 
			&thread_insert_worker_data[i]) != thrd_success) {
			printf("[TEST %s] Error: Failed to create insert worker thread", __func__);
			MM_DATA_FREE;
			ht_free(ht);
			return false;
		}
	}

	//--------------------------------------------------------------------------
	// Wait for threads to be completed.
	//--------------------------------------------------------------------------
	for(size_t i = 0; i < number_of_threads; i++) {
		thrd_join(threads_insert_worker[i], NULL);
	}
	thrd_join(thread_print_progresst_worker, NULL);

	//--------------------------------------------------------------------------
	// Print stats.
	//--------------------------------------------------------------------------
	printf("[TEST %s] Total keys inserted: %d\n", __func__, atomic_load(&keys_inserted));
	printf("[TEST %s] Total keys validated: %d\n", __func__, atomic_load(&keys_validated));
	printf("[TEST %s] Total keys removed: %d\n", __func__, atomic_load(&keys_removed));
	ht_print_stats(ht);

	//--------------------------------------------------------------------------
	// Release data. House-keeping.
	//--------------------------------------------------------------------------
	MM_DATA_FREE;
	ht_free(ht);
	printf("[TEST %s] PASSED successfully\n", __func__);
	return true;
}
