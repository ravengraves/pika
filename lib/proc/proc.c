/* pk_proc.c — backend-agnostic process helpers */

int32_t pk_proc_minprio(void) {
#ifdef _WIN32
	return -2;
#else
	return -20;
#endif
}

int32_t pk_proc_maxprio(void) {
#ifdef _WIN32
	return 2;
#else
	return 20;
#endif
}
