#include "hopscotch_ht.h"
#include "hopscotch_ht_test_iface.h"

int main() {
	test_run_concurrent(0x400000, murmur_custom_hash, false, 32);
	printf("\n");
	test_lookup_for_specific_key_value(NULL, murmur_custom_hash, 0x100000);
	printf("\n");
	test_insert_remove_elements(0x100000, murmur_custom_hash, false, true, true);
	printf("\n");
	test_relocation_and_max_relocation_value();
	return 0;
}
