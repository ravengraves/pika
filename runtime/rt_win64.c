/* Pika Runtime — Windows (MinGW) hosted version */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <io.h>
#include <process.h>
#include <stdlib.h>

/* Console API — declared directly to avoid pulling in all of windows.h */
#define WIN32_LEAN_AND_MEAN
typedef void* HANDLE;
typedef unsigned long DWORD;
typedef int BOOL;
__declspec(dllimport) HANDLE __stdcall GetStdHandle(DWORD nStdHandle);
__declspec(dllimport) BOOL __stdcall GetConsoleMode(HANDLE hConsole, DWORD* lpMode);
__declspec(dllimport) BOOL __stdcall SetConsoleMode(HANDLE hConsole, DWORD dwMode);
__declspec(dllimport) BOOL __stdcall SetConsoleOutputCP(unsigned int wCodePageID);
__declspec(dllimport) BOOL __stdcall SetConsoleCP(unsigned int wCodePageID);
#define RT_STD_OUTPUT_HANDLE ((DWORD)-11)
#define RT_STD_ERROR_HANDLE  ((DWORD)-12)
#define RT_ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004

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
 * I/O and exit — use Windows CRT
 * ========================================================================== */

static inline void rt_write_stderr(hare_str s) {
	if (s.data && s.len) {
		_write(2, s.data, (unsigned int)s.len);
	}
}

int64_t rt_write(int fd, const void* buf, size_t count) {
	return (int64_t)_write(fd, buf, (unsigned int)count);
}

int64_t rt_read(int fd, void* buf, size_t count) {
	return (int64_t)_read(fd, buf, (unsigned int)count);
}

__attribute__((noreturn))
void rt_exit(int status) {
	_exit(status);
	__builtin_trap();
}

/* ==========================================================================
 * Abort functions
 * ========================================================================== */

__attribute__((noreturn))
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
 * Entry point — not weak on Windows (MinGW CRT's main() calls WinMain())
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

_Bool pk_env_has(hare_str name) {
	char name_buf[4096];
	if (name.len >= sizeof(name_buf)) {
		return 0;
	}
	for (size_t i = 0; i < name.len; i++) {
		name_buf[i] = (char)name.data[i];
	}
	name_buf[name.len] = '\0';
	return getenv(name_buf) != NULL ? 1 : 0;
}

hare_str pk_env_get(hare_str name) {
	char name_buf[4096];
	if (name.len >= sizeof(name_buf)) {
		return (hare_str){0, 0};
	}
	for (size_t i = 0; i < name.len; i++) {
		name_buf[i] = (char)name.data[i];
	}
	name_buf[name.len] = '\0';
	char* value = getenv(name_buf);
	if (value == NULL) {
		return (hare_str){0, 0};
	}
	size_t n = 0;
	while (value[n] != '\0') n++;
	return (hare_str){(uint8_t*)value, n};
}

static void rt_console_init(void) {
	/* UTF-8 codepage for stdin and stdout — enables braille, block elements, CJK, emoji */
	SetConsoleOutputCP(65001);
	SetConsoleCP(65001);
	/* Enable ANSI/VT100 escape sequences on stdout and stderr */
	DWORD mode;
	HANDLE out = GetStdHandle(RT_STD_OUTPUT_HANDLE);
	if (out && GetConsoleMode(out, &mode)) {
		SetConsoleMode(out, mode | RT_ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}
	HANDLE err = GetStdHandle(RT_STD_ERROR_HANDLE);
	if (err && GetConsoleMode(err, &mode)) {
		SetConsoleMode(err, mode | RT_ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}
}

__attribute__((used))
int rt_start_c(int argc, char** argv) {
	rt_argc = argc;
	rt_argv = argv;
	rt_console_init();
	/* Set FTZ+DAZ for -ffast-math performance. */
#if defined(__x86_64__) || defined(__i386__)
	{
		unsigned int mxcsr;
		__asm__ volatile ("stmxcsr %0" : "=m"(mxcsr));
		mxcsr |= 0x8040; /* FTZ (bit 15) | DAZ (bit 6) */
		__asm__ volatile ("ldmxcsr %0" : : "m"(mxcsr));
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
