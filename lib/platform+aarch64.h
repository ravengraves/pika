/* libpka platform header — aarch64 Linux raw syscalls (-nostdlib -ffreestanding) */
#ifndef PK_PLATFORM_H
#define PK_PLATFORM_H

#include <stdint.h>
#include <stddef.h>

/* ========================================================================
 * Pika str/slice types — must match pika2c generated layout.
 * When compiled in the same TU as pika2c output, HARE_DEFINE_SLICE is
 * already defined and we alias pk_* to the hare_* types.
 * ======================================================================== */
#ifdef HARE_DEFINE_SLICE
/* Single TU: pika2c already defined hare_str etc. — alias them. */
typedef hare_str pk_str;
typedef hare_slice_uint8_t pk_slice_u8;
typedef hare_slice_hare_str pk_slice_str;
#else
/* Separate TU: define our own layout-compatible types. */
typedef struct { uint8_t* data; size_t len; } pk_str;
typedef struct { uint8_t* data; size_t len; size_t cap; } pk_slice_u8;
typedef struct { pk_str* data; size_t len; size_t cap; } pk_slice_str;
#endif

/* ========================================================================
 * Linux constants
 * ======================================================================== */

/* open(2) flags */
#define PK_O_RDONLY    0
#define PK_O_WRONLY    1
#define PK_O_RDWR     2
#define PK_O_CREAT    0x40
#define PK_O_TRUNC    0x200
#define PK_O_APPEND   0x400
#define PK_O_DIRECTORY 0x10000

/* *at() dirfd sentinel */
#define PK_AT_FDCWD            (-100)
#define PK_AT_SYMLINK_NOFOLLOW 0x100
#define PK_AT_REMOVEDIR        0x200

/* stat mode masks */
#define PK_S_IFMT   0170000
#define PK_S_IFREG  0100000
#define PK_S_IFDIR  0040000
#define PK_S_IFLNK  0120000

/* socket constants */
#define PK_AF_INET       2
#define PK_SOCK_STREAM   1
#define PK_SOCK_DGRAM    2
#define PK_SOL_SOCKET    1
#define PK_SO_REUSEADDR  2
#define PK_IPPROTO_TCP   6
#define PK_IPPROTO_UDP   17
#define PK_IP_ADD_MEMBERSHIP   35
#define PK_IP_DROP_MEMBERSHIP  36
#define PK_IPPROTO_IP    0

/* mmap */
#define PK_PROT_READ   0x1
#define PK_PROT_WRITE  0x2
#define PK_MAP_SHARED  0x01

/* clock */
#define PK_CLOCK_MONOTONIC 1

/* signals */
#define PK_SIGTERM  15
#define PK_SIGKILL  9
#define PK_SIGCHLD  17

/* prctl */
#define PK_PR_SET_PDEATHSIG 1

/* process priority */
#define PK_PRIO_PROCESS 0

/* wait */
#define PK_WNOHANG 1

/* lseek */
#define PK_SEEK_SET 0
#define PK_SEEK_CUR 1

/* ========================================================================
 * aarch64 Linux syscall numbers
 * ======================================================================== */
#define __PK_SYS_read            63
#define __PK_SYS_write           64
#define __PK_SYS_close           57
#define __PK_SYS_dup3            24
#define __PK_SYS_lseek           62
#define __PK_SYS_mmap            222
#define __PK_SYS_munmap          215
#define __PK_SYS_socket          198
#define __PK_SYS_connect         203
#define __PK_SYS_accept          202
#define __PK_SYS_sendto          206
#define __PK_SYS_recvfrom        207
#define __PK_SYS_bind            200
#define __PK_SYS_listen          201
#define __PK_SYS_getsockname     204
#define __PK_SYS_setsockopt      208
#define __PK_SYS_clone           220
#define __PK_SYS_execve          221
#define __PK_SYS_exit            93
#define __PK_SYS_wait4           260
#define __PK_SYS_kill            129
#define __PK_SYS_symlinkat       36
#define __PK_SYS_readlinkat      78
#define __PK_SYS_getpid          172
#define __PK_SYS_getuid          174
#define __PK_SYS_geteuid         175
#define __PK_SYS_nanosleep       101
#define __PK_SYS_getdents64      61
#define __PK_SYS_openat          56
#define __PK_SYS_mkdirat         34
#define __PK_SYS_fstatat         79
#define __PK_SYS_unlinkat        35
#define __PK_SYS_renameat2       276
#define __PK_SYS_getrandom       278
#define __PK_SYS_clock_gettime   113
#define __PK_SYS_ftruncate       46
#define __PK_SYS_fsync           82
#define __PK_SYS_prctl           167
#define __PK_SYS_getppid         173
#define __PK_SYS_getpriority     141
#define __PK_SYS_setpriority     140
#define __PK_SYS_shutdown        210
#define __PK_SYS_msync           227
#define __PK_SYS_ppoll           73

/* ========================================================================
 * Raw syscall wrappers — aarch64 uses svc #0, x8=nr, x0-x5=args, x0=ret
 * ======================================================================== */
static inline int64_t pk_syscall0(int64_t n) {
	register int64_t x8 __asm__("x8") = n;
	register int64_t x0 __asm__("x0");
	__asm__ volatile("svc #0" : "=r"(x0) : "r"(x8) : "memory");
	return x0;
}
static inline int64_t pk_syscall1(int64_t n, int64_t a1) {
	register int64_t x8 __asm__("x8") = n;
	register int64_t x0 __asm__("x0") = a1;
	__asm__ volatile("svc #0" : "+r"(x0) : "r"(x8) : "memory");
	return x0;
}
static inline int64_t pk_syscall2(int64_t n, int64_t a1, int64_t a2) {
	register int64_t x8 __asm__("x8") = n;
	register int64_t x0 __asm__("x0") = a1;
	register int64_t x1 __asm__("x1") = a2;
	__asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1) : "memory");
	return x0;
}
static inline int64_t pk_syscall3(int64_t n, int64_t a1, int64_t a2, int64_t a3) {
	register int64_t x8 __asm__("x8") = n;
	register int64_t x0 __asm__("x0") = a1;
	register int64_t x1 __asm__("x1") = a2;
	register int64_t x2 __asm__("x2") = a3;
	__asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
	return x0;
}
static inline int64_t pk_syscall4(int64_t n, int64_t a1, int64_t a2, int64_t a3, int64_t a4) {
	register int64_t x8 __asm__("x8") = n;
	register int64_t x0 __asm__("x0") = a1;
	register int64_t x1 __asm__("x1") = a2;
	register int64_t x2 __asm__("x2") = a3;
	register int64_t x3 __asm__("x3") = a4;
	__asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3) : "memory");
	return x0;
}
static inline int64_t pk_syscall5(int64_t n, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5) {
	register int64_t x8 __asm__("x8") = n;
	register int64_t x0 __asm__("x0") = a1;
	register int64_t x1 __asm__("x1") = a2;
	register int64_t x2 __asm__("x2") = a3;
	register int64_t x3 __asm__("x3") = a4;
	register int64_t x4 __asm__("x4") = a5;
	__asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4) : "memory");
	return x0;
}
static inline int64_t pk_syscall6(int64_t n, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5, int64_t a6) {
	register int64_t x8 __asm__("x8") = n;
	register int64_t x0 __asm__("x0") = a1;
	register int64_t x1 __asm__("x1") = a2;
	register int64_t x2 __asm__("x2") = a3;
	register int64_t x3 __asm__("x3") = a4;
	register int64_t x4 __asm__("x4") = a5;
	register int64_t x5 __asm__("x5") = a6;
	__asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) : "memory");
	return x0;
}

/* ========================================================================
 * Structs
 * ======================================================================== */

/* aarch64 Linux stat (128 bytes) — same layout as x86_64 for LP64 */
struct pk_linux_stat {
	uint64_t st_dev;
	uint64_t st_ino;
	uint64_t st_nlink;
	uint32_t st_mode;
	uint32_t st_uid;
	uint32_t st_gid;
	uint32_t __pad0;
	uint64_t st_rdev;
	int64_t  st_size;
	int64_t  st_blksize;
	int64_t  st_blocks;
	int64_t  st_atime;
	int64_t  st_atime_ns;
	int64_t  st_mtime;
	int64_t  st_mtime_ns;
	int64_t  st_ctime;
	int64_t  st_ctime_ns;
	int64_t  __unused[3];
};

struct pk_sockaddr_in {
	uint16_t family;
	uint16_t port;
	uint32_t addr;
	uint8_t  zero[8];
};

struct pk_ip_mreq {
	uint32_t multiaddr;
	uint32_t interface;
};

struct pk_timespec {
	int64_t sec;
	int64_t nsec;
};

struct pk_pollfd {
	int32_t fd;
	int16_t events;
	int16_t revents;
};

/* linux_dirent64 header: d_ino(8) + d_off(8) + d_reclen(2) + d_type(1) + d_name[] */
#define PK_DIRENT64_RECLEN_OFF 16
#define PK_DIRENT64_TYPE_OFF   18
#define PK_DIRENT64_NAME_OFF   19

/* DT_* values from linux */
#define PK_DT_UNKNOWN 0
#define PK_DT_REG     8
#define PK_DT_DIR     4
#define PK_DT_LNK     10

/* ========================================================================
 * Helper functions
 * ======================================================================== */

/* Null-terminate a pk_str into a stack buffer. Returns pointer to buf. */
static inline char* pk_str_to_cstr(pk_str s, char* buf, size_t bufsize) {
	size_t n = s.len < bufsize - 1 ? s.len : bufsize - 1;
	for (size_t i = 0; i < n; i++) buf[i] = (char)s.data[i];
	buf[n] = '\0';
	return buf;
}

/* Compare pk_str to a C string literal */
static inline int pk_str_eq_lit(pk_str s, const char* lit, size_t litlen) {
	if (s.len != litlen) return 0;
	for (size_t i = 0; i < litlen; i++) {
		if (s.data[i] != (uint8_t)lit[i]) return 0;
	}
	return 1;
}

/* Compare two pk_str */
static inline int pk_str_eq(pk_str a, pk_str b) {
	if (a.len != b.len) return 0;
	for (size_t i = 0; i < a.len; i++) {
		if (a.data[i] != b.data[i]) return 0;
	}
	return 1;
}

/* Byte-swap helpers */
static inline uint16_t pk_bswap16(uint16_t v) {
	return (uint16_t)((v << 8) | (v >> 8));
}
static inline uint32_t pk_bswap32(uint32_t v) {
	return (v >> 24) |
		((v >> 8) & 0x0000ff00u) |
		((v << 8) & 0x00ff0000u) |
		(v << 24);
}
static inline uint16_t pk_htons(uint16_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return pk_bswap16(v);
#else
	return v;
#endif
}
static inline uint32_t pk_htonl(uint32_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return pk_bswap32(v);
#else
	return v;
#endif
}

/* Parse dotted-decimal IPv4 "A.B.C.D" -> network-order u32. Returns 0 on failure. */
static inline uint32_t pk_parse_ipv4(pk_str s) {
	uint32_t parts[4] = {0,0,0,0};
	int part = 0;
	for (size_t i = 0; i < s.len && part < 4; i++) {
		uint8_t c = s.data[i];
		if (c == '.') {
			part++;
		} else if (c >= '0' && c <= '9') {
			parts[part] = parts[part] * 10 + (c - '0');
		} else {
			return 0;
		}
	}
	if (part != 3) return 0;
	for (int i = 0; i < 4; i++) {
		if (parts[i] > 255) return 0;
	}
	uint32_t host = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
	return pk_htonl(host);
}

/* DirNext cache for IsDir/IsFile/IsLink fast path */
static char pk__dir_last_name[256];
static size_t pk__dir_last_name_len = 0;
static uint8_t pk__dir_last_type = 0; /* 0=unknown, PK_DT_REG=file, PK_DT_DIR=dir, PK_DT_LNK=link */

/* ========================================================================
 * Tagged union result types and structs.
 * In single-TU mode (HARE_DEFINE_SLICE defined), pika2c already defined
 * these types. In separate-TU mode, we define layout-compatible versions
 * with the same names pika2c would generate.
 * ======================================================================== */

#ifndef HARE_DEFINE_SLICE
/* Separate TU: define all pika2c-generated types ourselves */

/* file module */
typedef struct { uint16_t tag; union { int32_t _0; intptr_t _1; } val; } file_OpenResult;
typedef struct { uint16_t tag; union { int64_t _0; intptr_t _1; } val; } file_ReadResult;
typedef struct { uint16_t tag; union { int64_t _0; intptr_t _1; } val; } file_WriteResult;
typedef struct { uint16_t tag; union { int64_t _0; intptr_t _1; } val; } file_SeekResult;
typedef struct { uint16_t tag; union { intptr_t _0; intptr_t _1; } val; } file_VoidResult;
typedef struct { uint16_t tag; union { int64_t _0; intptr_t _1; } val; } file_DirOpenResult;
typedef struct { uint16_t tag; union { pk_str _0; intptr_t _1; intptr_t _2; } val; } file_DirEntry;
typedef struct { uint16_t tag; union { int64_t _0; intptr_t _1; } val; } file_ReadLinkResult;

/* io module */
typedef struct { uint16_t tag; union { int16_t _0; intptr_t _1; intptr_t _2; } val; } io_PollResult;
typedef struct { uint16_t tag; union { int32_t _0; intptr_t _1; intptr_t _2; } val; } io_PollManyResult;
typedef struct { uint16_t tag; union { pk_str _0; intptr_t _1; intptr_t _2; } val; } io_InputResult;

/* proc module */
typedef struct { uint16_t tag; union { int32_t _0; intptr_t _1; } val; } proc_CreateResult;
typedef struct { uint16_t tag; union { int32_t _0; intptr_t _1; } val; } proc_Status;
typedef struct { uint16_t tag; union { intptr_t _0; intptr_t _1; } val; } proc_VoidResult;

/* net module */
typedef struct { uint16_t tag; union { int32_t _0; intptr_t _1; } val; } net_ListenResult;
typedef struct { uint16_t tag; union { int32_t _0; intptr_t _1; } val; } net_AcceptResult;
typedef struct { uint16_t tag; union { int32_t _0; intptr_t _1; } val; } net_ConnectResult;
typedef struct { uint16_t tag; union { int64_t _0; intptr_t _1; } val; } net_SendResult;
typedef struct { uint16_t tag; union { int64_t _0; intptr_t _1; } val; } net_RecvResult;
typedef struct { uint16_t tag; union { intptr_t _0; intptr_t _1; } val; } net_VoidResult;

/* shm module */
typedef struct { uint16_t tag; union { void* _0; intptr_t _1; } val; } shm_CreateResult;
typedef struct { uint16_t tag; union { void* _0; intptr_t _1; } val; } shm_OpenResult;

/* crypt module */
typedef struct { uint16_t tag; union { intptr_t _0; intptr_t _1; } val; } crypt_RandomResult;

/* fb module */
typedef struct { pk_slice_u8 buf; size_t index; } fb_Buffer;

/* io_PollFd for pollmany */
typedef struct { int32_t fd; int16_t events; int16_t revents; } io_PollFd;
typedef struct { io_PollFd* data; size_t len; size_t cap; } pk_slice_io_PollFd;

#endif /* !HARE_DEFINE_SLICE */

/* Slice type for pollmany in single-TU mode — pika2c generates this */
#ifdef HARE_DEFINE_SLICE
typedef hare_slice_io_PollFd pk_slice_io_PollFd;
#endif

#endif /* PK_PLATFORM_H */
