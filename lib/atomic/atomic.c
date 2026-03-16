// atomic — C backing (GCC __atomic_* builtins, backend-independent)

void pk_atomic_store(uint64_t* a, uint64_t val, int32_t order) {
	__atomic_store_n(a, val, order);
}

uint64_t pk_atomic_load(uint64_t* a, int32_t order) {
	return __atomic_load_n(a, order);
}

_Bool pk_atomic_cas(uint64_t* a, uint64_t* expected, uint64_t desired,
					int32_t success, int32_t failure) {
	/* Use strong CAS: weak CAS may fail spuriously on LL/SC targets like AArch64. */
	return __atomic_compare_exchange_n(a, expected, desired, 0, success, failure);
}
