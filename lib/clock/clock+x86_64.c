/* pk_clock.c — Clock module for x86_64 Linux raw syscalls */

uint64_t pk_clock_count(void) {
	uint32_t lo, hi;
	__asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
}

int64_t pk_clock_time_ns(void) {
	struct pk_timespec ts;
	int64_t ret = pk_syscall2(__PK_SYS_clock_gettime, PK_CLOCK_MONOTONIC, (int64_t)&ts);
	if (ret < 0) return -1;
	return ts.sec * 1000000000LL + ts.nsec;
}
