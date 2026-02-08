/* libpka platform header — x86_64 Linux raw syscalls (-nostdlib -ffreestanding) */
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
 * x86_64 Linux syscall numbers
 * ======================================================================== */
#define __PK_SYS_read            0
#define __PK_SYS_write           1
#define __PK_SYS_open            2
#define __PK_SYS_close           3
#define __PK_SYS_stat            4
#define __PK_SYS_fstat           5
#define __PK_SYS_lstat           6
#define __PK_SYS_poll            7
#define __PK_SYS_lseek           8
#define __PK_SYS_mmap            9
#define __PK_SYS_munmap          11
#define __PK_SYS_socket          41
#define __PK_SYS_connect         42
#define __PK_SYS_accept          43
#define __PK_SYS_sendto          44
#define __PK_SYS_recvfrom        45
#define __PK_SYS_bind            49
#define __PK_SYS_listen          50
#define __PK_SYS_getsockname     51
#define __PK_SYS_setsockopt      54
#define __PK_SYS_fork            57
#define __PK_SYS_execve          59
#define __PK_SYS_exit            60
#define __PK_SYS_wait4           61
#define __PK_SYS_kill            62
#define __PK_SYS_chdir           80
#define __PK_SYS_rename          82
#define __PK_SYS_mkdir           83
#define __PK_SYS_rmdir           84
#define __PK_SYS_unlink          87
#define __PK_SYS_symlink         88
#define __PK_SYS_readlink        89
#define __PK_SYS_getcwd          79
#define __PK_SYS_getpid          39
#define __PK_SYS_getuid          102
#define __PK_SYS_geteuid         107
#define __PK_SYS_nanosleep       35
#define __PK_SYS_getdents64      217
#define __PK_SYS_openat          257
#define __PK_SYS_mkdirat         258
#define __PK_SYS_fstatat         262
#define __PK_SYS_unlinkat        263
#define __PK_SYS_renameat2       316
#define __PK_SYS_getrandom       318
#define __PK_SYS_clock_gettime   228
#define __PK_SYS_ftruncate       77
#define __PK_SYS_fsync           74
#define __PK_SYS_prctl           157
#define __PK_SYS_getppid         110
#define __PK_SYS_getpriority     140
#define __PK_SYS_setpriority     141
#define __PK_SYS_shutdown        48
#define __PK_SYS_msync           26

/* ========================================================================
 * Raw syscall wrappers
 * ======================================================================== */
static inline int64_t pk_syscall0(int64_t n) {
	int64_t ret;
	__asm__ volatile("syscall" : "=a"(ret) : "a"(n) : "rcx","r11","memory");
	return ret;
}
static inline int64_t pk_syscall1(int64_t n, int64_t a1) {
	int64_t ret;
	__asm__ volatile("syscall" : "=a"(ret) : "a"(n),"D"(a1) : "rcx","r11","memory");
	return ret;
}
static inline int64_t pk_syscall2(int64_t n, int64_t a1, int64_t a2) {
	int64_t ret;
	__asm__ volatile("syscall" : "=a"(ret) : "a"(n),"D"(a1),"S"(a2) : "rcx","r11","memory");
	return ret;
}
static inline int64_t pk_syscall3(int64_t n, int64_t a1, int64_t a2, int64_t a3) {
	int64_t ret;
	__asm__ volatile("syscall" : "=a"(ret) : "a"(n),"D"(a1),"S"(a2),"d"(a3) : "rcx","r11","memory");
	return ret;
}
static inline int64_t pk_syscall4(int64_t n, int64_t a1, int64_t a2, int64_t a3, int64_t a4) {
	int64_t ret;
	register int64_t r10 __asm__("r10") = a4;
	__asm__ volatile("syscall" : "=a"(ret) : "a"(n),"D"(a1),"S"(a2),"d"(a3),"r"(r10) : "rcx","r11","memory");
	return ret;
}
static inline int64_t pk_syscall5(int64_t n, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5) {
	int64_t ret;
	register int64_t r10 __asm__("r10") = a4;
	register int64_t r8 __asm__("r8") = a5;
	__asm__ volatile("syscall" : "=a"(ret) : "a"(n),"D"(a1),"S"(a2),"d"(a3),"r"(r10),"r"(r8) : "rcx","r11","memory");
	return ret;
}
static inline int64_t pk_syscall6(int64_t n, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5, int64_t a6) {
	int64_t ret;
	register int64_t r10 __asm__("r10") = a4;
	register int64_t r8 __asm__("r8") = a5;
	register int64_t r9 __asm__("r9") = a6;
	__asm__ volatile("syscall" : "=a"(ret) : "a"(n),"D"(a1),"S"(a2),"d"(a3),"r"(r10),"r"(r8),"r"(r9) : "rcx","r11","memory");
	return ret;
}

/* ========================================================================
 * Structs
 * ======================================================================== */

/* x86_64 Linux stat (144 bytes) */
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

/* Parse dotted-decimal IPv4 "A.B.C.D" → network-order u32. Returns 0 on failure. */
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

/* fmt module */
typedef struct { pk_slice_u8 buf; size_t index; } fmt_Buffer;

/* io_PollFd for pollmany */
typedef struct { int32_t fd; int16_t events; int16_t revents; } io_PollFd;
typedef struct { io_PollFd* data; size_t len; size_t cap; } pk_slice_io_PollFd;

#endif /* !HARE_DEFINE_SLICE */

/* Slice type for pollmany in single-TU mode — pika2c generates this */
#ifdef HARE_DEFINE_SLICE
typedef hare_slice_io_PollFd pk_slice_io_PollFd;
#endif

#endif /* PK_PLATFORM_H */
