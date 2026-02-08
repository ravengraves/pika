/* pk_shm.c — Named shared memory for Windows (MinGW) */

static size_t pk_shm_build_name(pk_str name, char* buf, size_t bufsize) {
	/* Windows shared memory uses "Local\<name>" naming */
	const char prefix[] = "Local\\";
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

	char mapname[512];
	pk_shm_build_name(name, mapname, sizeof(mapname));

	/* Create file mapping backed by the system paging file */
	DWORD sizeHigh = (DWORD)((uint64_t)sz >> 32);
	DWORD sizeLow  = (DWORD)(sz & 0xFFFFFFFF);
	HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
		PAGE_READWRITE, sizeHigh, sizeLow, mapname);
	if (!hMap) { r.tag = 1; r.val._1 = 0; return r; }

	void* ptr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sz);
	if (!ptr) {
		CloseHandle(hMap);
		r.tag = 1; r.val._1 = 0; return r;
	}

	pk_str_to_cstr(name, pk_shm_table[slot].name, sizeof(pk_shm_table[slot].name));
	pk_shm_table[slot].hMapping = hMap;
	pk_shm_table[slot].ptr = ptr;
	pk_shm_table[slot].size = sz;
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

	char mapname[512];
	pk_shm_build_name(name, mapname, sizeof(mapname));

	HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, mapname);
	if (!hMap) { r.tag = 1; r.val._1 = 0; return r; }

	void* ptr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sz);
	if (!ptr) {
		CloseHandle(hMap);
		r.tag = 1; r.val._1 = 0; return r;
	}

	pk_str_to_cstr(name, pk_shm_table[slot].name, sizeof(pk_shm_table[slot].name));
	pk_shm_table[slot].hMapping = hMap;
	pk_shm_table[slot].ptr = ptr;
	pk_shm_table[slot].size = sz;
	pk_shm_table[slot].active = 1;

	r.tag = 0; r.val._0 = ptr;
	return r;
}

void pk_shm_close(pk_str name) {
	int slot = pk_shm_find(name);
	if (slot < 0) return;
	UnmapViewOfFile(pk_shm_table[slot].ptr);
	CloseHandle(pk_shm_table[slot].hMapping);
	pk_shm_table[slot].active = 0;
}

void pk_shm_del(pk_str name) {
	/* On Windows, named file mappings are reference-counted.
	 * They are destroyed when all handles are closed.
	 * There is no unlink equivalent. Close will suffice. */
	int slot = pk_shm_find(name);
	if (slot < 0) return;
	UnmapViewOfFile(pk_shm_table[slot].ptr);
	CloseHandle(pk_shm_table[slot].hMapping);
	pk_shm_table[slot].active = 0;
}
