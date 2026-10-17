/* libpka platform header — Windows 64-bit (MinGW/GCC) */
#ifndef PK_PLATFORM_H
#define PK_PLATFORM_H

#include <stdint.h>
#include <stddef.h>

/* Windows headers — order matters: winsock2 must come before windows.h */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <direct.h>

/* ========================================================================
 * Pika str/slice types — must match pika2c generated layout.
 * When compiled in the same TU as pika2c output, HARE_DEFINE_SLICE is
 * already defined and we alias pk_* to the hare_* types.
 * ======================================================================== */
#ifdef HARE_DEFINE_SLICE
/* Single TU: pika2c already defined hare_str etc. — alias them. */
typedef hare_str pk_str;
typedef hare_slice_uint8_t pk_slice_u8;
#ifdef PIKA_SLICE_DEF_hare_slice_hare_str
typedef hare_slice_hare_str pk_slice_str;
#else
typedef struct { hare_str* data; size_t len; size_t cap; } pk_slice_str;
#endif
#else
/* Separate TU: define our own layout-compatible types. */
typedef struct { uint8_t* data; size_t len; } pk_str;
typedef struct { uint8_t* data; size_t len; size_t cap; } pk_slice_u8;
typedef struct { pk_str* data; size_t len; size_t cap; } pk_slice_str;
#endif

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

/* ========================================================================
 * Directory listing cache
 * ======================================================================== */

/* DirNext cache for IsDir/IsFile/IsLink fast path */
static char pk__dir_last_name[256];
static size_t pk__dir_last_name_len = 0;
static uint8_t pk__dir_last_type = 0; /* 0=unknown, 1=file, 2=dir, 3=symlink */

#define PK_WIN_DT_UNKNOWN 0
#define PK_WIN_DT_REG     1
#define PK_WIN_DT_DIR     2
#define PK_WIN_DT_LNK     3

/* ========================================================================
 * Directory listing pool (8 slots)
 * ======================================================================== */
#define PK_MAX_OPEN_DIRS 8

typedef struct {
	HANDLE findHandle;
	WIN32_FIND_DATAA findData;
	int first; /* 1 = first entry not yet returned */
} pk_dir_state;

static pk_dir_state pk_dirs[PK_MAX_OPEN_DIRS];
static uint8_t pk_dirs_active[PK_MAX_OPEN_DIRS];

/* ========================================================================
 * Process handle table (64 slots)
 * ======================================================================== */
#define PK_MAX_PROCS 64

typedef struct {
	HANDLE hProcess;
	DWORD  dwPid;
	uint8_t active;
} pk_proc_entry;

static pk_proc_entry pk_proc_table[PK_MAX_PROCS];

static int pk_proc_find_by_pid(int32_t pid) {
	for (int i = 0; i < PK_MAX_PROCS; i++) {
		if (pk_proc_table[i].active && pk_proc_table[i].dwPid == (DWORD)pid)
			return i;
	}
	return -1;
}

/* ========================================================================
 * Winsock initialization
 * ======================================================================== */
static int pk__winsock_initialized = 0;

static inline void pk__ensure_winsock(void) {
	if (!pk__winsock_initialized) {
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 2), &wsa);
		pk__winsock_initialized = 1;
	}
}

/* ========================================================================
 * Job object for killing children when parent dies
 * ======================================================================== */
static HANDLE pk__job_object = NULL;

static inline void pk__ensure_job_object(void) {
	if (!pk__job_object) {
		pk__job_object = CreateJobObjectA(NULL, NULL);
		if (pk__job_object) {
			JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
			for (size_t i = 0; i < sizeof(info); i++) ((uint8_t*)&info)[i] = 0;
			info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			SetInformationJobObject(pk__job_object, JobObjectExtendedLimitInformation,
				&info, sizeof(info));
		}
	}
}

/* ========================================================================
 * Shared memory table
 * ======================================================================== */
#define PK_SHM_MAX 16

static struct {
	char name[256];
	HANDLE hMapping;
	void* ptr;
	size_t size;
	uint8_t active;
} pk_shm_table[PK_SHM_MAX];

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
typedef struct { uint16_t tag; union { pk_str _0; intptr_t _1; } val; } file_ReadLinkResult;

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
#ifdef PIKA_SLICE_DEF_hare_slice_io_PollFd
typedef hare_slice_io_PollFd pk_slice_io_PollFd;
#endif

#endif /* PK_PLATFORM_H */
