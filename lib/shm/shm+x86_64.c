/* pk_shm.c — Named shared memory for x86_64 Linux raw syscalls */

#define PK_SHM_MAX 16

static struct {
	char name[256];
	void* ptr;
	size_t size;
	int32_t fd;
	uint8_t active;
} pk_shm_table[PK_SHM_MAX];

static size_t pk_shm_build_path(pk_str name, char* buf, size_t bufsize) {
	const char prefix[] = "/dev/shm/";
	size_t plen = sizeof(prefix) - 1;
	size_t nlen = name.len < bufsize - plen - 1 ? name.len : bufsize - plen - 1;
	for (size_t i = 0; i < plen; i++) buf[i] = prefix[i];
	for (size_t i = 0; i < nlen; i++) buf[plen + i] = (char)name.data[i];
	buf[plen + nlen] = '\0';
	return plen + nlen;
}

static int pk_shm_find(pk_str name) {
	for (int i = 0; i < PK_SHM_MAX; i++) {
		if (!pk_shm_table[i].active) continue;
		size_t j = 0;
		while (j < name.len && pk_shm_table[i].name[j] == (char)name.data[j]) j++;
		if (j == name.len && pk_shm_table[i].name[j] == '\0') return i;
	}
	return -1;
}

shm_CreateResult pk_shm_create(pk_str name, size_t sz) {
	shm_CreateResult r;
	int slot = -1;
	for (int i = 0; i < PK_SHM_MAX; i++) {
		if (!pk_shm_table[i].active) { slot = i; break; }
	}
	if (slot < 0) { r.tag = 1; r.val._1 = 0; return r; }

	char path[512];
	pk_shm_build_path(name, path, sizeof(path));

	int64_t fd = pk_syscall4(__PK_SYS_openat, PK_AT_FDCWD, (int64_t)path,
		PK_O_CREAT | PK_O_RDWR | PK_O_TRUNC, 0666);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; return r; }

	if (pk_syscall2(__PK_SYS_ftruncate, (int32_t)fd, (int64_t)sz) < 0) {
		pk_syscall1(__PK_SYS_close, (int32_t)fd);
		r.tag = 1; r.val._1 = 0; return r;
	}

	int64_t ptr = pk_syscall6(__PK_SYS_mmap, 0, (int64_t)sz,
		PK_PROT_READ | PK_PROT_WRITE, PK_MAP_SHARED, (int32_t)fd, 0);
	if (ptr < 0 || ptr == -1) {
		pk_syscall1(__PK_SYS_close, (int32_t)fd);
		r.tag = 1; r.val._1 = 0; return r;
	}

	pk_str_to_cstr(name, pk_shm_table[slot].name, sizeof(pk_shm_table[slot].name));
	pk_shm_table[slot].ptr = (void*)ptr;
	pk_shm_table[slot].size = sz;
	pk_shm_table[slot].fd = (int32_t)fd;
	pk_shm_table[slot].active = 1;

	r.tag = 0; r.val._0 = (void*)ptr;
	return r;
}

shm_OpenResult pk_shm_open(pk_str name, size_t sz) {
	shm_OpenResult r;
	int slot = -1;
	for (int i = 0; i < PK_SHM_MAX; i++) {
		if (!pk_shm_table[i].active) { slot = i; break; }
	}
	if (slot < 0) { r.tag = 1; r.val._1 = 0; return r; }

	char path[512];
	pk_shm_build_path(name, path, sizeof(path));

	int64_t fd = pk_syscall4(__PK_SYS_openat, PK_AT_FDCWD, (int64_t)path, PK_O_RDWR, 0);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; return r; }

	int64_t ptr = pk_syscall6(__PK_SYS_mmap, 0, (int64_t)sz,
		PK_PROT_READ | PK_PROT_WRITE, PK_MAP_SHARED, (int32_t)fd, 0);
	if (ptr < 0 || ptr == -1) {
		pk_syscall1(__PK_SYS_close, (int32_t)fd);
		r.tag = 1; r.val._1 = 0; return r;
	}

	pk_str_to_cstr(name, pk_shm_table[slot].name, sizeof(pk_shm_table[slot].name));
	pk_shm_table[slot].ptr = (void*)ptr;
	pk_shm_table[slot].size = sz;
	pk_shm_table[slot].fd = (int32_t)fd;
	pk_shm_table[slot].active = 1;

	r.tag = 0; r.val._0 = (void*)ptr;
	return r;
}

void pk_shm_close(pk_str name) {
	int slot = pk_shm_find(name);
	if (slot < 0) return;
	pk_syscall2(__PK_SYS_munmap, (int64_t)pk_shm_table[slot].ptr, (int64_t)pk_shm_table[slot].size);
	pk_syscall1(__PK_SYS_close, pk_shm_table[slot].fd);
	pk_shm_table[slot].active = 0;
}

void pk_shm_del(pk_str name) {
	char path[512];
	pk_shm_build_path(name, path, sizeof(path));
	pk_syscall3(__PK_SYS_unlinkat, PK_AT_FDCWD, (int64_t)path, 0);
}
