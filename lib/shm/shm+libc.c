/* pk_shm.c — Named shared memory using libc (POSIX) */

#define PK_SHM_MAX 16

static struct {
	char name[256];
	void* ptr;
	size_t size;
	int fd;
	uint8_t active;
} pk_shm_table[PK_SHM_MAX];

/* Build shm name: "/" prefix for shm_open, "/dev/shm/" for Linux open() fallback */
static size_t pk_shm_build_name(pk_str name, char* buf, size_t bufsize) {
	buf[0] = '/';
	size_t nlen = name.len < bufsize - 2 ? name.len : bufsize - 2;
	for (size_t i = 0; i < nlen; i++) buf[1 + i] = (char)name.data[i];
	buf[1 + nlen] = '\0';
	return 1 + nlen;
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

	char shmname[512];
	pk_shm_build_name(name, shmname, sizeof(shmname));

	int fd = shm_open(shmname, O_CREAT | O_RDWR | O_TRUNC, 0666);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; return r; }

	if (ftruncate(fd, (off_t)sz) < 0) {
		close(fd);
		r.tag = 1; r.val._1 = 0; return r;
	}

	void* ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		close(fd);
		r.tag = 1; r.val._1 = 0; return r;
	}

	pk_str_to_cstr(name, pk_shm_table[slot].name, sizeof(pk_shm_table[slot].name));
	pk_shm_table[slot].ptr = ptr;
	pk_shm_table[slot].size = sz;
	pk_shm_table[slot].fd = fd;
	pk_shm_table[slot].active = 1;

	r.tag = 0; r.val._0 = ptr;
	return r;
}

shm_OpenResult pk_shm_open(pk_str name, size_t sz) {
	shm_OpenResult r;
	int slot = -1;
	for (int i = 0; i < PK_SHM_MAX; i++) {
		if (!pk_shm_table[i].active) { slot = i; break; }
	}
	if (slot < 0) { r.tag = 1; r.val._1 = 0; return r; }

	char shmname[512];
	pk_shm_build_name(name, shmname, sizeof(shmname));

	int fd = shm_open(shmname, O_RDWR, 0);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; return r; }

	void* ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		close(fd);
		r.tag = 1; r.val._1 = 0; return r;
	}

	pk_str_to_cstr(name, pk_shm_table[slot].name, sizeof(pk_shm_table[slot].name));
	pk_shm_table[slot].ptr = ptr;
	pk_shm_table[slot].size = sz;
	pk_shm_table[slot].fd = fd;
	pk_shm_table[slot].active = 1;

	r.tag = 0; r.val._0 = ptr;
	return r;
}

void pk_shm_close(pk_str name) {
	int slot = pk_shm_find(name);
	if (slot < 0) return;
	munmap(pk_shm_table[slot].ptr, pk_shm_table[slot].size);
	close(pk_shm_table[slot].fd);
	pk_shm_table[slot].active = 0;
}

void pk_shm_del(pk_str name) {
	char shmname[512];
	pk_shm_build_name(name, shmname, sizeof(shmname));
	shm_unlink(shmname);
}
