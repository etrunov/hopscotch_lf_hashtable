#include "hopscotch_ht.h"

//------------------------------------------------------------------------------
// Hash functions related block.
//------------------------------------------------------------------------------
// For any hash function no key pointer check to speed-up the performance.
// MurmurHash3 custom.
inline uint32_t murmur_custom_hash(const uint8_t* key) {
	const uint64_t* k = (const uint64_t*)key;
	uint64_t h = k[0] ^ 0x9E3779B185EBCA87;  // Initial mix
	// This is not 0xDEADBEEF :)
	// Process 64-byte key in 8-byte chunks
	h = (h ^ k[1]) * 0xC6BC279692B5CC83;
	h = (h ^ k[2]) * 0x9E3779B97F4A7C15;
	h = (h ^ k[3]) * 0xC6BC279692B5CC83;
	h = (h ^ k[4]) * 0x9E3779B185EBCA87;
	h = (h ^ k[5]) * 0xC6BC279692B5CC83;
	h = (h ^ k[6]) * 0x9E3779B97F4A7C15;
	h = (h ^ k[7]) * 0xC6BC279692B5CC83;

	// Final mixing for better avalanche
	h ^= h >> 33;
	h *= 0xFF51AFD7ED558CCD;
	h ^= h >> 33;

	return (uint32_t)(h ^ (h >> 32));
}

// Jenkins hash function.
inline uint32_t jenkins_one_at_a_time_hash(const uint8_t *key) {
	uint32_t hash = 0;

	// This is not 0xBAADF00D :)
	for(size_t i = 0; i < KEY_SIZE; ++i) {
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}

// Dummy hash function to test collisions.
// Suppress any warrning related messages.
inline uint32_t dummy_set_1_hash(const uint8_t *key __attribute__((unused))) {
	// This is 0xDEADFA11
	return (uint32_t)1;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Hash table related functions / API.
//------------------------------------------------------------------------------
void ht_print_debug(const hopscotch_hash_table_t * const ht) {
	if(!ht) return;

	static _Atomic int print_lock = 0;

	// Spinlock for thread-safe printing.
	while (atomic_exchange_explicit(&print_lock, 1, memory_order_acquire)) {
		thrd_yield();
	}

	printf("\nHopscotch Hash Table (Capacity: %zu, Size: %zu)\n",
		   ht->capacity, atomic_load_explicit(&ht->size, memory_order_relaxed));
	printf("-----------------------------------------------------------------------------------------\n");
	printf("IDX   Hom->Cur Hash     Hop bits     Key....  Val....  Neighborhood(32)\n");
	printf("-----------------------------------------------------------------------------------------\n");

	for(size_t i = 0; i < ht->capacity; i++) {
		uint64_t node_info = atomic_load_explicit(&ht->nodes[i].hop_info, memory_order_acquire);
		uint32_t node_hash = (uint32_t)(node_info >> 32);

		// Maintaining only occupied buckets.
		if(node_hash != 0) {  // Only show occupied buckets
			uint32_t hop_bits = (uint32_t)(node_info & 0xFFFFFFFF);
			size_t home = node_hash & ht->mask;

			// IDX - Home->Curr - Hash.
			printf("[%03zu] %03zu->%03zu %08X ", i, home, i, node_hash);

			// Hop Bits.
			for(int j = 3; j >= 0; j--) {
				printf("%02X", (hop_bits >> (j*8)) & 0xFF);
				if(j > 0) printf(".");
			}
			
			// Key - Value.
			printf("  %02X%02X...  %02X%02X...  ", 
				   ht->nodes[i].key[0], ht->nodes[i].key[1],
				   ht->nodes[i].value[0], ht->nodes[i].value[1]);
			
			// Neighborhood (32). Neighborhood visualization.
			printf("[");
			uint64_t neighbor_info = atomic_load_explicit(&ht->nodes[i].hop_info, memory_order_relaxed);
			uint32_t neighbor_hash = (uint32_t)(neighbor_info >> 32);
			uint32_t hop_info = neighbor_info & 0x00000000FFFFFFFF;
			
			for(size_t j = 0; j < HOP_RANGE; j++) {
				uint32_t mask = 1u << j;
				if(hop_info & mask) {
					printf("x");  // Current bucket
				} else if(neighbor_hash != 0) {
					 printf(".");
				} 
			}
			printf("]\n");
		}
	}
	atomic_store_explicit(&print_lock, 0, memory_order_release);
}

void ht_print_stats(const hopscotch_hash_table_t * const ht) {
	if(!ht) return;
	size_t size = atomic_load_explicit(&ht->size, memory_order_relaxed);
	printf("Hash table stats: size=%zu (%.1f%% full)\n", 
			size, (size * 100.0) / ht->capacity);
}

void ht_zero(hopscotch_hash_table_t *ht) {
	if(!ht) return;
	memset(ht->nodes, 0, ht->capacity * sizeof(hash_node_t));
	atomic_init(&ht->size, 0);
}

hopscotch_hash_table_t *ht_create(size_t capacity) {
	if(capacity == 0) return NULL;

	// Calculate total memory needed.
	size_t total_size = sizeof(hopscotch_hash_table_t) +
						(capacity * sizeof(hash_node_t));
	
	// Allocate single contiguous block.
	uint8_t* buffer = aligned_alloc(64, total_size);
	if(!buffer) return NULL;
	
	hopscotch_hash_table_t *ht = (hopscotch_hash_table_t *)buffer;
	ht->nodes = (hash_node_t *)(buffer + sizeof(hopscotch_hash_table_t));
	ht->capacity = capacity;
	ht->mask = capacity - 1;

	// Initialize nodes
	ht_zero(ht);
	return ht;
}

void ht_free(hopscotch_hash_table_t *ht) {
	if(!ht) return;

	free(ht);
	ht = NULL;
}

bool ht_insert(
	hopscotch_hash_table_t* ht,
	hash_function_f hash_key,
	const uint8_t *key,
	const uint8_t *value
) {
	uint32_t h = hash_key(key);
	size_t home = INDEX(h, ht->mask); // number of buckets (mask = capacity - 1)

	// Check for existing key first.
	for(size_t i = 0; i < HOP_RANGE; i++) {
		size_t idx = (home + i) & ht->mask;
		uint64_t node_info = atomic_load_explicit(
			&ht->nodes[idx].hop_info, memory_order_acquire);
		if((node_info >> HASH_HOP_INFO_OFFSET) == h) {
			if(memcmp(ht->nodes[idx].key, key, KEY_SIZE) == 0) {
				// Update existing.
				memcpy(ht->nodes[idx].value, value, VALUE_SIZE);
				return true;
			}
		}
	}

	// Find empty slot in neighborhood.
	for(size_t i = 0; i < HOP_RANGE; i++) {
		size_t idx = (home + i) & ht->mask;
		uint64_t current = atomic_load(&ht->nodes[idx].hop_info);
		if((current >> HASH_HOP_INFO_OFFSET) == 0) { // Empty slot
			uint64_t desired = ((uint64_t)h << HASH_HOP_INFO_OFFSET) | (1ULL << i);
			if(atomic_compare_exchange_weak_explicit(
				&ht->nodes[idx].hop_info, 
				&current, 
				desired,
				memory_order_release,
				memory_order_acquire))
			{
				memcpy(ht->nodes[idx].key, key, KEY_SIZE);
				memcpy(ht->nodes[idx].value, value, VALUE_SIZE);
				atomic_fetch_add(&ht->size, 1);
				return true;
			}
		}
	}

	// Need to relocate entries to make space.
	size_t free_slot = home;
	while (
		(free_slot < ht->capacity) && 
		(free_slot < home + MAX_RELOCATION_FACTOR * HOP_RANGE))
	{
		uint64_t current = atomic_load(&ht->nodes[free_slot].hop_info);
		if((current >> HASH_HOP_INFO_OFFSET) == 0) break;
		free_slot++;
	}

	if(
		(((free_slot - home) & ht->mask) >= HOP_RANGE) &&
		(((free_slot - home) & ht->mask) < HOP_RANGE * MAX_RELOCATION_FACTOR)

	) {
		// Perform relocation.
		// TODO: Additional logic but if it is required for the algo.
	} else {
		return false; // Table may not be fully full but range is full.
	}

	if(free_slot >= ht->capacity) {
		// Table is full.
		return false;
	}

	// Perform hopscotch relocation.
	while (free_slot >= home + MAX_RELOCATION_FACTOR * HOP_RANGE) {
		bool moved = false;

		for(size_t i = 0; i < HOP_RANGE; i++) {
			size_t candidate = (free_slot - HOP_RANGE + 1 + i) & ht->mask;
			uint64_t candidate_info = atomic_load_explicit(
					&ht->nodes[candidate].hop_info,
					memory_order_acquire);

			// Lower 32 bits (hop bits)
			uint32_t candidate_hop = candidate_info & HOP_INFO_MASK;
			
			if(candidate_hop != 0) {
				size_t first_hop = __builtin_ctz(candidate_hop);
				size_t move_from = (candidate + first_hop) & ht->mask;
				
				// Load the original hash before moving.
				uint64_t original_hash = atomic_load_explicit(
						&ht->nodes[move_from].hop_info,
						memory_order_acquire) & HASH_MASK;
				
				// Move the entry to free slot.
				memcpy(
					&ht->nodes[free_slot],
					&ht->nodes[move_from],
					sizeof(hash_node_t)
				);
				
				// Update the moved entry's hop_info:
				// Save original hash (upper 32 bits).
				// Set new hop bit relative to its home bucket.
				size_t new_home = (hash_key(ht->nodes[free_slot].key) & ht->mask);
				uint64_t new_hop_bit = 1ULL << (new_home - 1);
				atomic_store_explicit(&ht->nodes[free_slot].hop_info,
									original_hash | new_hop_bit,
									memory_order_release);

				// Update candidate's hop bitmap.
				uint64_t old_val, new_val;
				do {
					old_val = atomic_load(&ht->nodes[candidate].hop_info);
					if(free_slot - candidate >= HOP_RANGE) assert(0);
					uint32_t new_hop = ((old_val & HOP_INFO_MASK) ^ (1UL << first_hop)) | 
									(1UL << (free_slot - candidate));
					if(new_hop == 0) assert(0);
					new_val = (old_val & HASH_MASK) | new_hop;
				} while (!atomic_compare_exchange_weak_explicit(
					&ht->nodes[candidate].hop_info,
					&old_val,
					new_val,
					memory_order_release,
					memory_order_acquire
				));

				// Clear old slot.
				atomic_store_explicit(&ht->nodes[move_from].hop_info, 0, memory_order_release);
				memset(&ht->nodes[move_from].key, 0, KEY_SIZE);
				memset(&ht->nodes[move_from].value, 0, VALUE_SIZE);

				free_slot = move_from;
				moved = true;
				break;
			}
		}
		if(!moved) {
			printf("Unable to find a candidate for relocation\n");
			return false;
		} else {
			// Relocation was done without issues.
			break;
		}
	}

	// Now insert in the freed slot.
	uint64_t desired = (
		(uint64_t)h << HASH_HOP_INFO_OFFSET) |
		(1ULL << (free_slot - home) % HOP_RANGE);
	atomic_store_explicit(&ht->nodes[free_slot].hop_info, desired, memory_order_release);
	memcpy(ht->nodes[free_slot].key, key, KEY_SIZE);
	memcpy(ht->nodes[free_slot].value, value, VALUE_SIZE);
	atomic_fetch_add_explicit(&ht->size, 1, memory_order_relaxed);
	return true;
}

bool ht_remove_key(
	hopscotch_hash_table_t *ht,
	hash_function_f hash_function,
	const uint8_t *key
) {
	uint32_t h = hash_function(key);
	size_t home = INDEX(h, ht->mask);

	// Search in the neighborhood for the key.
	for(size_t i = 0; i < HOP_RANGE * MAX_RELOCATION_FACTOR; i++) {
		size_t idx = (home + i) & ht->mask;
		uint64_t node_info = atomic_load_explicit(
			&ht->nodes[idx].hop_info,
			memory_order_acquire
		);
		
		if((node_info >> HASH_HOP_INFO_OFFSET) == h) {
			if(memcmp(ht->nodes[idx].key, key, KEY_SIZE) == 0) {
				// Found the key, now remove it.
				// Clear the hop bit in the home bucket.
				size_t home_idx = home;
				uint64_t old_val, new_val;
				do {
					old_val = atomic_load_explicit(
						&ht->nodes[home_idx].hop_info,
						memory_order_acquire
					);
					uint32_t new_hop = old_val & HOP_INFO_MASK;
					new_hop &= ~(1UL << i % HOP_RANGE);  // Clear the bit for this index
					new_val = (old_val & HASH_MASK) | new_hop;
				} while (!atomic_compare_exchange_weak_explicit(
					&ht->nodes[home_idx].hop_info,
					&old_val,
					new_val,
					memory_order_release,
					memory_order_acquire
				));

				// Clear the node's data
				atomic_store_explicit(&ht->nodes[idx].hop_info, 0, memory_order_release);
				memset(ht->nodes[idx].key, 0, KEY_SIZE);
				memset(ht->nodes[idx].value, 0, VALUE_SIZE);

				// Decrement size
				atomic_fetch_sub_explicit(&ht->size, 1, memory_order_relaxed);
				return true;
			}
		}
	}
	return false; // Key not found
	}

bool ht_contains_key(
	const hopscotch_hash_table_t *ht,
	hash_function_f hash_function,
	const uint8_t *key,
	uint8_t *out_value
) {
	uint32_t h = hash_function(key);
	size_t home = h & ht->mask;

	for(size_t i = 0; i < HOP_RANGE * MAX_RELOCATION_FACTOR; i++) {
		size_t idx = (home + i) & ht->mask;

		// Atomic load matches ht_remove_key's function.
		uint64_t node_info = atomic_load_explicit(
			&ht->nodes[idx].hop_info,
			memory_order_acquire);

			uint32_t node_hash = (uint32_t)(node_info >> HASH_HOP_INFO_OFFSET);

		if(node_hash == h) {
			bool match = true;
			for(int j = 0; j < KEY_SIZE; j++) {
				uint8_t key_byte = atomic_load_explicit(
					&ht->nodes[idx].key[j],
					memory_order_relaxed);
				if(key_byte != key[j]) {
					match = false;
					break;
				}
			}

			if(match) {
				// Only difference is optional value retrieval.
				if(out_value) {
					for(int j = 0; j < VALUE_SIZE; j++) {
						out_value[j] = atomic_load_explicit(
							&ht->nodes[idx].value[j],
							memory_order_relaxed);
					}
				}
				return true;
			}
		}
	}
	return false;
}

// !DO NOT USE!
// This is non-atomic !non-thread-safe! Exposed to compare with atomic variants
// to estimate complexity of the code.
bool __ht_contains(hopscotch_hash_table_t* ht, const uint8_t* key) {
	uint32_t h = murmur_custom_hash(key);
	size_t home = h % ht->capacity;

	for(int i = 0; i < HOP_RANGE * MAX_RELOCATION_FACTOR; i++) {
		size_t idx = (home + i) % ht->capacity;
		uint64_t node_info = atomic_load(&ht->nodes[idx].hop_info);
		
		// Skip empty slots (full hash == 0)
		if((node_info >> 32) == 0) continue;
		
		// Full key comparison (critical!)
		if(memcmp(ht->nodes[idx].key, key, KEY_SIZE) == 0) {
			return true;
		}
	}
	return false;
}
//------------------------------------------------------------------------------
