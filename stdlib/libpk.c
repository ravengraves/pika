/* libpk - minimal Linux syscall-backed runtime for Pika */
// thanks to: https://takashiidobe.com/gen/writing-your-own-libc

#include <stdint.h>
#include <stddef.h>

#if defined(__linux__)
#include <asm/unistd.h>
#endif

struct pk_timespec {
	int64_t sec;
	int64_t nsec;
};

#if defined(__linux__) && defined(__x86_64__)
static inline long pk_syscall0(long n) {
	long ret;
	__asm__ volatile (
		"syscall"
		: "=a"(ret)
		: "a"(n)
		: "rcx", "r11", "memory"
	);
	return ret;
}
static inline long pk_syscall1(long n, long a1) {
	long ret;
	__asm__ volatile (
		"syscall"
		: "=a"(ret)
		: "a"(n), "D"(a1)
		: "rcx", "r11", "memory"
	);
	return ret;
}
static inline long pk_syscall2(long n, long a1, long a2) {
	long ret;
	__asm__ volatile (
		"syscall"
		: "=a"(ret)
		: "a"(n), "D"(a1), "S"(a2)
		: "rcx", "r11", "memory"
	);
	return ret;
}
static inline long pk_syscall3(long n, long a1, long a2, long a3) {
	long ret;
	__asm__ volatile (
		"syscall"
		: "=a"(ret)
		: "a"(n), "D"(a1), "S"(a2), "d"(a3)
		: "rcx", "r11", "memory"
	);
	return ret;
}
static inline long pk_syscall4(long n, long a1, long a2, long a3, long a4) {
	long ret;
	register long r10 __asm__("r10") = a4;
	__asm__ volatile (
		"syscall"
		: "=a"(ret)
		: "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
		: "rcx", "r11", "memory"
	);
	return ret;
}
static inline long pk_syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
	long ret;
	register long r10 __asm__("r10") = a4;
	register long r8 __asm__("r8") = a5;
	__asm__ volatile (
		"syscall"
		: "=a"(ret)
		: "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
		: "rcx", "r11", "memory"
	);
	return ret;
}
static inline long pk_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
	long ret;
	register long r10 __asm__("r10") = a4;
	register long r8 __asm__("r8") = a5;
	register long r9 __asm__("r9") = a6;
	__asm__ volatile (
		"syscall"
		: "=a"(ret)
		: "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
		: "rcx", "r11", "memory"
	);
	return ret;
}

#elif defined(__linux__) && defined(__aarch64__)
static inline long pk_syscall0(long n) {
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0");
	__asm__ volatile (
		"svc #0"
		: "=r"(x0)
		: "r"(x8)
		: "memory"
	);
	return x0;
}
static inline long pk_syscall1(long n, long a1) {
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = a1;
	__asm__ volatile (
		"svc #0"
		: "+r"(x0)
		: "r"(x8)
		: "memory"
	);
	return x0;
}
static inline long pk_syscall2(long n, long a1, long a2) {
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = a1;
	register long x1 __asm__("x1") = a2;
	__asm__ volatile (
		"svc #0"
		: "+r"(x0)
		: "r"(x8), "r"(x1)
		: "memory"
	);
	return x0;
}
static inline long pk_syscall3(long n, long a1, long a2, long a3) {
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = a1;
	register long x1 __asm__("x1") = a2;
	register long x2 __asm__("x2") = a3;
	__asm__ volatile (
		"svc #0"
		: "+r"(x0)
		: "r"(x8), "r"(x1), "r"(x2)
		: "memory"
	);
	return x0;
}
static inline long pk_syscall4(long n, long a1, long a2, long a3, long a4) {
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = a1;
	register long x1 __asm__("x1") = a2;
	register long x2 __asm__("x2") = a3;
	register long x3 __asm__("x3") = a4;
	__asm__ volatile (
		"svc #0"
		: "+r"(x0)
		: "r"(x8), "r"(x1), "r"(x2), "r"(x3)
		: "memory"
	);
	return x0;
}
static inline long pk_syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = a1;
	register long x1 __asm__("x1") = a2;
	register long x2 __asm__("x2") = a3;
	register long x3 __asm__("x3") = a4;
	register long x4 __asm__("x4") = a5;
	__asm__ volatile (
		"svc #0"
		: "+r"(x0)
		: "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4)
		: "memory"
	);
	return x0;
}
static inline long pk_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = a1;
	register long x1 __asm__("x1") = a2;
	register long x2 __asm__("x2") = a3;
	register long x3 __asm__("x3") = a4;
	register long x4 __asm__("x4") = a5;
	register long x5 __asm__("x5") = a6;
	__asm__ volatile (
		"svc #0"
		: "+r"(x0)
		: "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5)
		: "memory"
	);
	return x0;
}

#else
static inline long pk_syscall0(long n) { (void)n; return -1; }
static inline long pk_syscall1(long n, long a1) { (void)n; (void)a1; return -1; }
static inline long pk_syscall2(long n, long a1, long a2) { (void)n; (void)a1; (void)a2; return -1; }
static inline long pk_syscall3(long n, long a1, long a2, long a3) { (void)n; (void)a1; (void)a2; (void)a3; return -1; }
static inline long pk_syscall4(long n, long a1, long a2, long a3, long a4) {
	(void)n; (void)a1; (void)a2; (void)a3; (void)a4; return -1;
}
static inline long pk_syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
	(void)n; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; return -1;
}
static inline long pk_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
	(void)n; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6; return -1;
}
#endif

uint64_t pk_clock_count(void) {
#if defined(__x86_64__)
	uint32_t lo;
	uint32_t hi;
	__asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
	uint64_t v;
	__asm__ volatile ("mrs %0, cntvct_el0" : "=r"(v));
	return v;
#elif defined(__riscv) && (__riscv_xlen == 64)
	uint64_t v;
	__asm__ volatile ("rdcycle %0" : "=r"(v));
	return v;
#else
	return 0;
#endif
}

void* pk_map_failed(void) {
	return (void*)-1;
}

int64_t pk_read(int32_t fd, void* buf, size_t count) {
#if defined(__linux__) && defined(__NR_read)
	return (int64_t)pk_syscall3(__NR_read, fd, (long)buf, (long)count);
#else
	(void)fd; (void)buf; (void)count;
	return -1;
#endif
}

int64_t pk_write(int32_t fd, void* buf, size_t count) {
#if defined(__linux__) && defined(__NR_write)
	return (int64_t)pk_syscall3(__NR_write, fd, (long)buf, (long)count);
#else
	(void)fd; (void)buf; (void)count;
	return -1;
#endif
}

int32_t pk_open(uint8_t* path, int32_t flags, uint32_t mode) {
#if defined(__linux__) && defined(__NR_openat)
	return (int32_t)pk_syscall4(__NR_openat, -100, (long)path, flags, mode);
#elif defined(__linux__) && defined(__NR_open)
	return (int32_t)pk_syscall3(__NR_open, (long)path, flags, mode);
#else
	(void)path; (void)flags; (void)mode;
	return -1;
#endif
}

int32_t pk_close(int32_t fd) {
#if defined(__linux__) && defined(__NR_close)
	return (int32_t)pk_syscall1(__NR_close, fd);
#else
	(void)fd;
	return -1;
#endif
}

int32_t pk_fsync(int32_t fd) {
#if defined(__linux__) && defined(__NR_fsync)
	return (int32_t)pk_syscall1(__NR_fsync, fd);
#else
	(void)fd;
	return -1;
#endif
}

int32_t pk_ftruncate(int32_t fd, int64_t size) {
#if defined(__linux__) && defined(__NR_ftruncate)
	return (int32_t)pk_syscall2(__NR_ftruncate, fd, (long)size);
#else
	(void)fd; (void)size;
	return -1;
#endif
}

void* pk_mmap(void* addr, size_t length, int32_t prot, int32_t flags, int32_t fd, int64_t offset) {
#if defined(__linux__) && defined(__NR_mmap)
	long ret = pk_syscall6(__NR_mmap, (long)addr, (long)length, prot, flags, fd, (long)offset);
	return (void*)ret;
#else
	(void)addr; (void)length; (void)prot; (void)flags; (void)fd; (void)offset;
	return pk_map_failed();
#endif
}

int32_t pk_munmap(void* addr, size_t length) {
#if defined(__linux__) && defined(__NR_munmap)
	return (int32_t)pk_syscall2(__NR_munmap, (long)addr, (long)length);
#else
	(void)addr; (void)length;
	return -1;
#endif
}

int32_t pk_msync(void* addr, size_t length, int32_t flags) {
#if defined(__linux__) && defined(__NR_msync)
	return (int32_t)pk_syscall3(__NR_msync, (long)addr, (long)length, flags);
#else
	(void)addr; (void)length; (void)flags;
	return -1;
#endif
}

int32_t pk_socket(int32_t domain, int32_t kind, int32_t proto) {
#if defined(__linux__) && defined(__NR_socket)
	return (int32_t)pk_syscall3(__NR_socket, domain, kind, proto);
#else
	(void)domain; (void)kind; (void)proto;
	return -1;
#endif
}

int32_t pk_connect(int32_t fd, void* addr, size_t addrlen) {
#if defined(__linux__) && defined(__NR_connect)
	return (int32_t)pk_syscall3(__NR_connect, fd, (long)addr, (long)addrlen);
#else
	(void)fd; (void)addr; (void)addrlen;
	return -1;
#endif
}

int32_t pk_bind(int32_t fd, void* addr, size_t addrlen) {
#if defined(__linux__) && defined(__NR_bind)
	return (int32_t)pk_syscall3(__NR_bind, fd, (long)addr, (long)addrlen);
#else
	(void)fd; (void)addr; (void)addrlen;
	return -1;
#endif
}

int32_t pk_listen(int32_t fd, int32_t backlog) {
#if defined(__linux__) && defined(__NR_listen)
	return (int32_t)pk_syscall2(__NR_listen, fd, backlog);
#else
	(void)fd; (void)backlog;
	return -1;
#endif
}

int32_t pk_accept(int32_t fd, void* addr, uint32_t* addrlen) {
#if defined(__linux__) && defined(__NR_accept)
	return (int32_t)pk_syscall3(__NR_accept, fd, (long)addr, (long)addrlen);
#else
	(void)fd; (void)addr; (void)addrlen;
	return -1;
#endif
}

int32_t pk_getsockname(int32_t fd, void* addr, uint32_t* addrlen) {
#if defined(__linux__) && defined(__NR_getsockname)
	return (int32_t)pk_syscall3(__NR_getsockname, fd, (long)addr, (long)addrlen);
#else
	(void)fd; (void)addr; (void)addrlen;
	return -1;
#endif
}

int64_t pk_send(int32_t fd, void* buf, size_t len, int32_t flags) {
#if defined(__linux__) && defined(__NR_sendto)
	return (int64_t)pk_syscall6(__NR_sendto, fd, (long)buf, (long)len, flags, 0, 0);
#else
	(void)fd; (void)buf; (void)len; (void)flags;
	return -1;
#endif
}

int64_t pk_recv(int32_t fd, void* buf, size_t len, int32_t flags) {
#if defined(__linux__) && defined(__NR_recvfrom)
	return (int64_t)pk_syscall6(__NR_recvfrom, fd, (long)buf, (long)len, flags, 0, 0);
#else
	(void)fd; (void)buf; (void)len; (void)flags;
	return -1;
#endif
}

int32_t pk_shutdown(int32_t fd, int32_t how) {
#if defined(__linux__) && defined(__NR_shutdown)
	return (int32_t)pk_syscall2(__NR_shutdown, fd, how);
#else
	(void)fd; (void)how;
	return -1;
#endif
}

int32_t pk_getppid(void) {
#if defined(__linux__) && defined(__NR_getppid)
	return (int32_t)pk_syscall0(__NR_getppid);
#else
	return -1;
#endif
}

int32_t pk_kill(int32_t pid, int32_t sig) {
#if defined(__linux__) && defined(__NR_kill)
	return (int32_t)pk_syscall2(__NR_kill, pid, sig);
#else
	(void)pid; (void)sig;
	return -1;
#endif
}

int32_t pk_prctl(int32_t option, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5) {
#if defined(__linux__) && defined(__NR_prctl)
	return (int32_t)pk_syscall5(__NR_prctl, option, (long)arg2, (long)arg3, (long)arg4, (long)arg5);
#else
	(void)option; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
	return -1;
#endif
}

int32_t pk_wait4(int32_t pid, int32_t* status, int32_t options) {
#if defined(__linux__) && defined(__NR_wait4)
	return (int32_t)pk_syscall4(__NR_wait4, pid, (long)status, options, 0);
#else
	(void)pid; (void)status; (void)options;
	return -1;
#endif
}

int32_t pk_spawn(uint8_t* path, uint8_t* const* argv, uint8_t* const* envp) {
#if defined(__linux__) && defined(__NR_fork)
	int32_t pid = (int32_t)pk_syscall0(__NR_fork);
	if (pid != 0) { return pid; }
#if defined(__NR_execve)
	pk_syscall3(__NR_execve, (long)path, (long)argv, (long)envp);
#else
	(void)path; (void)argv; (void)envp;
#endif
#if defined(__NR_exit)
	pk_syscall1(__NR_exit, 127);
#endif
	return -1;
#else
	(void)path; (void)argv; (void)envp;
	return -1;
#endif
}

int32_t pk_getpriority(int32_t which, int32_t who) {
#if defined(__linux__) && defined(__NR_getpriority)
	return (int32_t)pk_syscall2(__NR_getpriority, which, who);
#else
	(void)which; (void)who;
	return -1;
#endif
}

int32_t pk_setpriority(int32_t which, int32_t who, int32_t prio) {
#if defined(__linux__) && defined(__NR_setpriority)
	return (int32_t)pk_syscall3(__NR_setpriority, which, who, prio);
#else
	(void)which; (void)who; (void)prio;
	return -1;
#endif
}

int32_t pk_nanosleep(struct pk_timespec* req, struct pk_timespec* rem) {
#if defined(__linux__) && defined(__NR_nanosleep)
	return (int32_t)pk_syscall2(__NR_nanosleep, (long)req, (long)rem);
#else
	(void)req; (void)rem;
	return -1;
#endif
}

int32_t pk_clock_gettime(int32_t clock_id, struct pk_timespec* ts) {
#if defined(__linux__) && defined(__NR_clock_gettime)
	return (int32_t)pk_syscall2(__NR_clock_gettime, clock_id, (long)ts);
#elif defined(__linux__) && defined(__NR_clock_gettime64)
	return (int32_t)pk_syscall2(__NR_clock_gettime64, clock_id, (long)ts);
#else
	(void)clock_id; (void)ts;
	return -1;
#endif
}

__attribute__((noreturn))
void pk_exit(int32_t status) {
#if defined(__linux__) && defined(__NR_exit)
	pk_syscall1(__NR_exit, status);
#endif
	__builtin_trap();
}

static inline uint16_t pk_bswap16(uint16_t v) {
	return (uint16_t)((v << 8) | (v >> 8));
}
static inline uint32_t pk_bswap32(uint32_t v) {
	return (v >> 24) |
		((v >> 8) & 0x0000ff00u) |
		((v << 8) & 0x00ff0000u) |
		(v << 24);
}

uint16_t pk_htons(uint16_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return pk_bswap16(v);
#else
	return v;
#endif
}

uint16_t pk_ntohs(uint16_t v) {
	return pk_htons(v);
}

uint32_t pk_htonl(uint32_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return pk_bswap32(v);
#else
	return v;
#endif
}

uint32_t pk_ntohl(uint32_t v) {
	return pk_htonl(v);
}

/* stat structure for x86_64 Linux (144 bytes) */
struct pk_stat {
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

#define PK_AT_FDCWD           -100
#define PK_AT_SYMLINK_NOFOLLOW 0x100
#define PK_AT_REMOVEDIR        0x200
#define PK_O_DIRECTORY         0x10000

#define PK_S_IFMT   0170000
#define PK_S_IFREG  0100000
#define PK_S_IFDIR  0040000
#define PK_S_IFLNK  0120000

int32_t pk_fstatat(int32_t dirfd, uint8_t* path, struct pk_stat* buf, int32_t flags) {
#if defined(__linux__) && defined(__NR_newfstatat)
	return (int32_t)pk_syscall4(__NR_newfstatat, dirfd, (long)path, (long)buf, flags);
#elif defined(__linux__) && defined(__NR_fstatat64)
	return (int32_t)pk_syscall4(__NR_fstatat64, dirfd, (long)path, (long)buf, flags);
#else
	(void)dirfd; (void)path; (void)buf; (void)flags;
	return -1;
#endif
}

int32_t pk_fstat(int32_t fd, struct pk_stat* buf) {
#if defined(__linux__) && defined(__NR_fstat)
	return (int32_t)pk_syscall2(__NR_fstat, fd, (long)buf);
#else
	(void)fd; (void)buf;
	return -1;
#endif
}

int32_t pk_mkdirat(int32_t dirfd, uint8_t* path, uint32_t mode) {
#if defined(__linux__) && defined(__NR_mkdirat)
	return (int32_t)pk_syscall3(__NR_mkdirat, dirfd, (long)path, mode);
#else
	(void)dirfd; (void)path; (void)mode;
	return -1;
#endif
}

int32_t pk_unlinkat(int32_t dirfd, uint8_t* path, int32_t flags) {
#if defined(__linux__) && defined(__NR_unlinkat)
	return (int32_t)pk_syscall3(__NR_unlinkat, dirfd, (long)path, flags);
#else
	(void)dirfd; (void)path; (void)flags;
	return -1;
#endif
}

int32_t pk_chdir(uint8_t* path) {
#if defined(__linux__) && defined(__NR_chdir)
	return (int32_t)pk_syscall1(__NR_chdir, (long)path);
#else
	(void)path;
	return -1;
#endif
}

int32_t pk_getcwd(uint8_t* buf, size_t size) {
#if defined(__linux__) && defined(__NR_getcwd)
	long ret = pk_syscall2(__NR_getcwd, (long)buf, (long)size);
	if (ret < 0) return -1;
	/* getcwd returns the length including null terminator, we return without it */
	size_t len = 0;
	while (len < size && buf[len] != 0) len++;
	return (int32_t)len;
#else
	(void)buf; (void)size;
	return -1;
#endif
}

int64_t pk_getdents64(int32_t fd, void* buf, size_t size) {
#if defined(__linux__) && defined(__NR_getdents64)
	return (int64_t)pk_syscall3(__NR_getdents64, fd, (long)buf, (long)size);
#else
	(void)fd; (void)buf; (void)size;
	return -1;
#endif
}

/* Readdir state: one active directory at a time for simplicity */
static uint8_t pk_dirent_buf[4096];
static size_t pk_dirent_pos = 0;
static size_t pk_dirent_len = 0;
static int pk_dirent_fd = -1;

int32_t pk_readdir(int32_t fd, uint8_t* name, size_t size, uint8_t* kind) {
	/* Reset buffer if different fd */
	if (fd != pk_dirent_fd) {
		pk_dirent_fd = fd;
		pk_dirent_pos = 0;
		pk_dirent_len = 0;
	}

	/* Refill buffer if exhausted */
	if (pk_dirent_pos >= pk_dirent_len) {
		int64_t ret = pk_getdents64(fd, pk_dirent_buf, sizeof(pk_dirent_buf));
		if (ret <= 0) {
			pk_dirent_fd = -1;
			return (ret == 0) ? 0 : -1;
		}
		pk_dirent_len = (size_t)ret;
		pk_dirent_pos = 0;
	}

	/* Parse dirent64: d_ino(8), d_off(8), d_reclen(2), d_type(1), d_name[] */
	uint8_t* ent = &pk_dirent_buf[pk_dirent_pos];
	uint16_t reclen = *(uint16_t*)(ent + 16);
	uint8_t d_type = ent[18];
	uint8_t* d_name = ent + 19;

	/* Copy name */
	size_t namelen = 0;
	while (d_name[namelen] != 0 && namelen < size - 1) {
		name[namelen] = d_name[namelen];
		namelen++;
	}
	name[namelen] = 0;

	if (kind) *kind = d_type;
	pk_dirent_pos += reclen;

	return (int32_t)namelen;
}

void pk_readdir_reset(void) {
	pk_dirent_fd = -1;
	pk_dirent_pos = 0;
	pk_dirent_len = 0;
}

int32_t pk_getuid(void) {
#if defined(__linux__) && defined(__NR_getuid)
	return (int32_t)pk_syscall0(__NR_getuid);
#else
	return -1;
#endif
}

int32_t pk_geteuid(void) {
#if defined(__linux__) && defined(__NR_geteuid)
	return (int32_t)pk_syscall0(__NR_geteuid);
#else
	return -1;
#endif
}

int32_t pk_getgid(void) {
#if defined(__linux__) && defined(__NR_getgid)
	return (int32_t)pk_syscall0(__NR_getgid);
#else
	return -1;
#endif
}

int32_t pk_getegid(void) {
#if defined(__linux__) && defined(__NR_getegid)
	return (int32_t)pk_syscall0(__NR_getegid);
#else
	return -1;
#endif
}

int32_t pk_getpid(void) {
#if defined(__linux__) && defined(__NR_getpid)
	return (int32_t)pk_syscall0(__NR_getpid);
#else
	return -1;
#endif
}

int64_t pk_getrandom(void* buf, size_t buflen, uint32_t flags) {
#if defined(__linux__) && defined(__NR_getrandom)
	return (int64_t)pk_syscall3(__NR_getrandom, (long)buf, (long)buflen, flags);
#else
	(void)buf; (void)buflen; (void)flags;
	return -1;
#endif
}

int64_t pk_lseek(int32_t fd, int64_t offset, int32_t whence) {
#if defined(__linux__) && defined(__NR_lseek)
	return (int64_t)pk_syscall3(__NR_lseek, fd, (long)offset, whence);
#else
	(void)fd; (void)offset; (void)whence;
	return -1;
#endif
}

struct pk_pollfd {
	int32_t fd;
	int16_t events;
	int16_t revents;
};

int32_t pk_poll(struct pk_pollfd* fds, size_t nfds, int32_t timeout_ms) {
#if defined(__linux__) && defined(__NR_poll)
	return (int32_t)pk_syscall3(__NR_poll, (long)fds, (long)nfds, timeout_ms);
#elif defined(__linux__) && defined(__NR_ppoll)
	/* ppoll with NULL sigmask and converted timeout */
	if (timeout_ms < 0) {
		return (int32_t)pk_syscall5(__NR_ppoll, (long)fds, (long)nfds, 0, 0, 0);
	}
	struct pk_timespec ts;
	ts.sec = timeout_ms / 1000;
	ts.nsec = (timeout_ms % 1000) * 1000000L;
	return (int32_t)pk_syscall5(__NR_ppoll, (long)fds, (long)nfds, (long)&ts, 0, 0);
#else
	(void)fds; (void)nfds; (void)timeout_ms;
	return -1;
#endif
}
