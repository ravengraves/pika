/* pk_clock.c — Clock module for Windows (MinGW) */

uint64_t pk_clock_count(void) {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return (uint64_t)li.QuadPart;
}

int64_t pk_clock_time_ns(void) {
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	/* Convert to nanoseconds: count * 1e9 / freq
	 * Split to avoid overflow: whole seconds + remainder */
	int64_t sec = count.QuadPart / freq.QuadPart;
	int64_t rem = count.QuadPart % freq.QuadPart;
	return sec * 1000000000LL + (rem * 1000000000LL) / freq.QuadPart;
}
