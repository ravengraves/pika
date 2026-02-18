/* pk_math.c - C-backed helpers for math module */

int32_t pk_math_popcount(uint64_t x) {
	return __builtin_popcountll((unsigned long long)x);
}
