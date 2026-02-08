/* pk_clock.c — Clock module using libc (POSIX) */

uint64_t pk_clock_count(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int64_t pk_clock_time_ns(void) {
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) return -1;
	return (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
}
