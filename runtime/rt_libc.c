/* Pika Runtime — hosted libc version (Linux, macOS, BSD) */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef __linux__
#include <sys/prctl.h>
#endif

/* String type matching Hare's str.
 * Guard: when catted into unified build, pika2c already defines these types. */
#ifndef HARE_DEFINE_SLICE
typedef struct { uint8_t* data; size_t len; } hare_str;
#endif

static int rt_argc = 0;
static char** rt_argv = NULL;

/* Error tag base for tagged unions */
#define HARE_ERROR_TAG_BASE 0x80000000U

/* Fixed error messages as hare_str (data, len) */
#define HARE_STR_LIT(s) { (uint8_t*)(s), sizeof(s) - 1 }
static const hare_str rt_fixed_errors[] = {
	HARE_STR_LIT("slice or array access out of bounds"),                  /* 0 */
	HARE_STR_LIT("type assertion failed"),                                /* 1 */
	HARE_STR_LIT("out of memory"),                                        /* 2 */
	HARE_STR_LIT("static insert/append exceeds slice capacity"),          /* 3 */
	HARE_STR_LIT("execution reached unreachable code (compiler bug)"),    /* 4 */
	HARE_STR_LIT("slice allocation capacity smaller than initializer"),   /* 5 */
	HARE_STR_LIT("assertion failed"),                                     /* 6 */
	HARE_STR_LIT("error occurred"),                                       /* 7 */
};
static const hare_str rt_unknown_error = HARE_STR_LIT("unknown error");
static const hare_str rt_abort_prefix = HARE_STR_LIT("abort: ");
static const hare_str rt_newline = HARE_STR_LIT("\n");

/* ==========================================================================
 * Core memory functions — delegate to libc
 * ========================================================================== */

void rt_memcpy(void* __restrict__ dest, const void* __restrict__ src, size_t n) {
	memcpy(dest, src, n);
}

void rt_memmove(void* dest, const void* src, size_t n) {
	memmove(dest, src, n);
}

void rt_memset(void* dest, int c, size_t n) {
	memset(dest, c, n);
}

int rt_memcmp(const void* a, const void* b, size_t n) {
	return memcmp(a, b, n);
}

int rt_strcmp(hare_str a, hare_str b) {
	if (a.len != b.len) return 0;
	return memcmp(a.data, b.data, a.len) == 0 ? 1 : 0;
}

/* ==========================================================================
 * I/O and exit — use libc
 * ========================================================================== */

static inline void rt_write_stderr(hare_str s) {
	if (s.data && s.len) {
		write(2, s.data, s.len);
	}
}

int64_t rt_write(int fd, const void* buf, size_t count) {
	return (int64_t)write(fd, buf, count);
}

int64_t rt_read(int fd, void* buf, size_t count) {
	return (int64_t)read(fd, buf, count);
}

__attribute__((noreturn))
void rt_exit(int status) {
	_exit(status);
	__builtin_trap();
}

/* ==========================================================================
 * Abort functions
 * ========================================================================== */

__attribute__((weak, noreturn))
void rt_abort_handler(hare_str path, uint64_t line, uint64_t col, hare_str msg) {
	(void)path; (void)line; (void)col;
	rt_write_stderr(rt_abort_prefix);
	rt_write_stderr(msg);
	rt_write_stderr(rt_newline);
	rt_exit(134);
	__builtin_trap();
}

__attribute__((noreturn))
void rt_abort(hare_str* path, uint64_t line, uint64_t col, hare_str msg) {
	hare_str p = path ? *path : (hare_str){0, 0};
	rt_abort_handler(p, line, col, msg);
	__builtin_unreachable();
}

__attribute__((noreturn))
void rt_abort_fixed(hare_str* path, uint64_t line, uint64_t col, uint64_t i) {
	hare_str msg = (i < sizeof(rt_fixed_errors)/sizeof(rt_fixed_errors[0]))
	             ? rt_fixed_errors[i] : rt_unknown_error;
	hare_str p = path ? *path : (hare_str){0, 0};
	rt_abort_handler(p, line, col, msg);
	__builtin_unreachable();
}

/* ==========================================================================
 * Entry point
 * ========================================================================== */

extern void _main(void);

int32_t pk_args_count(void) {
	return (int32_t)rt_argc;
}

hare_str pk_args_at(int32_t index) {
	if (index < 0 || index >= (int32_t)rt_argc || rt_argv == NULL || rt_argv[index] == NULL) {
		return (hare_str){0, 0};
	}
	const char* s = rt_argv[index];
	size_t n = 0;
	while (s[n] != '\0') n++;
	return (hare_str){(uint8_t*)s, n};
}

__attribute__((used))
int rt_start_c(int argc, char** argv) {
	rt_argc = argc;
	rt_argv = argv;
	/* Linux safety: kill this process if its parent exits. */
#ifdef __linux__
	prctl(PR_SET_PDEATHSIG, 15 /* SIGTERM */);
	if (getppid() == 1) _exit(0);
#endif
	/* Set FTZ+DAZ for -ffast-math performance.
	 * Without this, denormal floats cause ~100x pipeline stalls.
	 * Normally crtfastmath.o does this, but some toolchains skip it. */
#if defined(__x86_64__) || defined(__i386__)
	{
		unsigned int mxcsr;
		__asm__ volatile ("stmxcsr %0" : "=m"(mxcsr));
		mxcsr |= 0x8040; /* FTZ (bit 15) | DAZ (bit 6) */
		__asm__ volatile ("ldmxcsr %0" : : "m"(mxcsr));
	}
#elif defined(__aarch64__)
	{
		unsigned long fpcr;
		__asm__ volatile ("mrs %0, fpcr" : "=r"(fpcr));
		fpcr |= (1UL << 24); /* FZ bit — flush denormals to zero */
		__asm__ volatile ("msr fpcr, %0" : : "r"(fpcr));
	}
#endif
	_main();
	return 0;
}

int main(int argc, char** argv) {
	rt_start_c(argc, argv);
	return 0;
}

/* ==========================================================================
 * strings:: module - string/byte slice conversions
 * ========================================================================== */

#ifndef HARE_DEFINE_SLICE
typedef struct {
	uint8_t* data;
	size_t len;
	size_t cap;
} hare_slice_uint8_t;
#endif

hare_slice_uint8_t strings_toutf8(hare_str s) {
	return (hare_slice_uint8_t){ s.data, s.len, s.len };
}

hare_str strings_fromutf8_unsafe(hare_slice_uint8_t in) {
	return (hare_str){ in.data, in.len };
}
