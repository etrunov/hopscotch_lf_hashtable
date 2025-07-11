#include "hopscotch_ht_test_misc.h"
#include "hopscotch_ht.h"

//------------------------------------------------------------------------------
// Test data generation functions.
//------------------------------------------------------------------------------
double get_current_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec * 1e-6;
}

//------------------------------------------------------------------------------
// Test data generation functions.
//------------------------------------------------------------------------------
int generate_random_pair(uint8_t *key, uint8_t *value) {
	if(!key && !value) {
		printf("Error: keys and values is zero");
		return 0;
	}

	// open  /dev/urandom.
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd == -1) {
		printf("Error: Unable to open /dev/urandom");
		return 1;
	}

	// Allocate memory.
	unsigned char *buffer = malloc(KEY_SIZE + VALUE_SIZE);
	if (!buffer) {
		printf("Error: Unable to allocate memory\n");
		close(fd);
		return 1;
	}

	size_t bytes_read = read(fd, buffer, KEY_SIZE + VALUE_SIZE);
	if (bytes_read != KEY_SIZE + VALUE_SIZE) {
		perror("Error: Unable to read /dev/urandom\n");
		free(buffer);
		close(fd);
		return 1;
	}

	memcpy(key, buffer, KEY_SIZE);
	memcpy(value, buffer + KEY_SIZE, VALUE_SIZE);

	// Release resources.
	free(buffer);
	close(fd);
	return 0;
}

test_data_t *allocate_test_data(size_t size) {
	test_data_t *pdata = malloc(sizeof(test_data_t) * size);
	if(!pdata) {
		printf("Error: Unable to allocate Keys and Values\n");
		return NULL;
	}

	for(size_t i = 0; i < size; i++) {
		// Initialize current element.
		pdata[i].key = (uint8_t *)malloc(sizeof(uint8_t) * KEY_SIZE);
		if(pdata[i].key == NULL) {
			printf("Error: Unable to allocate Key\n");
			// Free all previously allocated memory before returning.
			for(size_t j = 0; j < i; j++) {
				free(pdata[j].key);
				free(pdata[j].value);
			}
			free(pdata);
			pdata = NULL;
			return NULL;
		}

		pdata[i].value = (uint8_t *)malloc(sizeof(uint8_t) * VALUE_SIZE);
		if(pdata[i].value == NULL) {
			printf("Error: Unable to allocate Value\n");
			free(pdata[i].key);
			// Free all previously allocated memory before returning.
			for(size_t j = 0; j < i; j++) {
				free(pdata[j].key);
				free(pdata[j].value);
			}
			free(pdata);
			pdata = NULL;
			return NULL;
		}

		if(generate_random_pair(pdata[i].key, pdata[i].value) != 0) {
			free(pdata[i].key);
			free(pdata[i].value);
			// Free all previously allocated memory before returning.
			for(size_t j = 0; j < i; j++) {
				free(pdata[j].key);
				free(pdata[j].value);
			}
			free(pdata);
			pdata = NULL;
			printf("Error: Unable to generate random Key-Value\n");
			return NULL;
		}
		pdata[i].inserted = false;
	}
	return pdata;
}

void free_test_data(test_data_t *pdata, size_t size) {
	if (pdata == NULL) {
		return;  // Nothing to free
	}

	for (size_t i = 0; i < size; i++) {
		if (pdata[i].key != NULL) {
			free(pdata[i].key);
			pdata[i].key = NULL;
		}
		if (pdata[i].value != NULL) {
			free(pdata[i].value);
			pdata[i].value = NULL;
		}
	}
	free(pdata);
	pdata = NULL;
}

//------------------------------------------------------------------------------
// Other functions.
//------------------------------------------------------------------------------
// Round up to the next highest power of 2 (for 32-bit numbers).
uint32_t round_to_power_of_two_32(uint32_t v) {
	if (v == 0) return 1;  // Edge case: 0 → 1
	v--;
	v |= v >> 1; v |= v >> 2;
	v |= v >> 4; v |= v >> 8;
	v |= v >> 16;
	return v + 1;
}

// Round up to the next highest power of 2 (for 64-bit numbers).
uint64_t round_to_power_of_two_64(uint64_t v) {
	if (v == 0) return 1;  // Edge case: 0 → 1
	v--;
	v |= v >> 1;  v |= v >> 2;
	v |= v >> 4;  v |= v >> 8;
	v |= v >> 16; v |= v >> 32;
	return v + 1;
}

// Generic version (auto-selects 32/64 based on size_t).
size_t round_to_power_of_two(size_t v) {
	if (sizeof(size_t) == 4) return (size_t)round_to_power_of_two_32((uint32_t)v);
	else return (size_t)round_to_power_of_two_64((uint64_t)v);
}
