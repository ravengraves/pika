/* pk_file.c — File operations for Windows (MinGW) */

/* ========================================================================
 * Internal helpers
 * ======================================================================== */

static int pk_mode_to_flags(pk_str mode) {
	if (pk_str_eq_lit(mode, "r", 1))
		return _O_RDONLY | _O_BINARY;
	if (pk_str_eq_lit(mode, "w", 1))
		return _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY;
	if (pk_str_eq_lit(mode, "a", 1))
		return _O_WRONLY | _O_CREAT | _O_APPEND | _O_BINARY;
	if (pk_str_eq_lit(mode, "rw", 2))
		return _O_RDWR | _O_CREAT | _O_BINARY;
	if (pk_str_eq_lit(mode, "rw+", 3))
		return _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY;
	return -1;
}

/* ========================================================================
 * Core I/O
 * ======================================================================== */

file_OpenResult pk_file_open(pk_str path, pk_str mode) {
	file_OpenResult r;
	int flags = pk_mode_to_flags(mode);
	if (flags < 0) { r.tag = 1; r.val._1 = 0; return r; }
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	int fd = _open(cbuf, flags, _S_IREAD | _S_IWRITE);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int32_t)fd; }
	return r;
}

void pk_file_close(int32_t fd) {
	_close(fd);
}

file_ReadResult pk_file_read(int32_t fd, pk_slice_u8 buf) {
	file_ReadResult r;
	int n = _read(fd, buf.data, (unsigned int)buf.len);
	if (n < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int64_t)n; }
	return r;
}

file_WriteResult pk_file_write(int32_t fd, pk_slice_u8 data) {
	file_WriteResult r;
	int n = _write(fd, data.data, (unsigned int)data.len);
	if (n < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int64_t)n; }
	return r;
}

file_SeekResult pk_file_seek(int32_t fd, int64_t pos) {
	file_SeekResult r;
	int64_t ret = _lseeki64(fd, pos, SEEK_SET);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = ret; }
	return r;
}

int64_t pk_file_loc(int32_t fd) {
	return _lseeki64(fd, 0, SEEK_CUR);
}

/* ========================================================================
 * Filesystem operations
 * ======================================================================== */

file_VoidResult pk_file_del(pk_str path) {
	file_VoidResult r;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	if (DeleteFileA(cbuf)) { r.tag = 0; r.val._0 = 0; }
	else { r.tag = 1; r.val._1 = 0; }
	return r;
}

file_VoidResult pk_file_rename(pk_str old_path, pk_str new_path) {
	file_VoidResult r;
	char cbuf1[4096], cbuf2[4096];
	pk_str_to_cstr(old_path, cbuf1, sizeof(cbuf1));
	pk_str_to_cstr(new_path, cbuf2, sizeof(cbuf2));
	if (MoveFileExA(cbuf1, cbuf2, MOVEFILE_REPLACE_EXISTING)) {
		r.tag = 0; r.val._0 = 0;
	} else {
		r.tag = 1; r.val._1 = 0;
	}
	return r;
}

file_VoidResult pk_file_mkdir(pk_str path) {
	file_VoidResult r;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	if (CreateDirectoryA(cbuf, NULL)) { r.tag = 0; r.val._0 = 0; }
	else { r.tag = 1; r.val._1 = 0; }
	return r;
}

file_VoidResult pk_file_rmdir(pk_str path) {
	file_VoidResult r;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	if (RemoveDirectoryA(cbuf)) { r.tag = 0; r.val._0 = 0; }
	else { r.tag = 1; r.val._1 = 0; }
	return r;
}

/* ========================================================================
 * Stat-based queries
 * ======================================================================== */

int64_t pk_file_size(pk_str path) {
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	struct __stat64 st;
	if (_stat64(cbuf, &st) != 0) return -1;
	return (int64_t)st.st_size;
}

_Bool pk_file_exists(pk_str path) {
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	DWORD attr = GetFileAttributesA(cbuf);
	return attr != INVALID_FILE_ATTRIBUTES;
}

_Bool pk_file_is_dir(pk_str path) {
	if (pk_file_dir_cache_match(path) && pk__dir_last_type == PK_WIN_DT_DIR) return 1;
	if (pk_file_dir_cache_match(path) && pk__dir_last_type != PK_WIN_DT_UNKNOWN) return 0;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	DWORD attr = GetFileAttributesA(cbuf);
	if (attr == INVALID_FILE_ATTRIBUTES) return 0;
	return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

_Bool pk_file_is_file(pk_str path) {
	if (pk_file_dir_cache_match(path) && pk__dir_last_type == PK_WIN_DT_REG) return 1;
	if (pk_file_dir_cache_match(path) && pk__dir_last_type != PK_WIN_DT_UNKNOWN) return 0;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	DWORD attr = GetFileAttributesA(cbuf);
	if (attr == INVALID_FILE_ATTRIBUTES) return 0;
	return (attr & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == 0;
}

_Bool pk_file_is_link(pk_str path) {
	if (pk_file_dir_cache_match(path) && pk__dir_last_type == PK_WIN_DT_LNK) return 1;
	if (pk_file_dir_cache_match(path) && pk__dir_last_type != PK_WIN_DT_UNKNOWN) return 0;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	DWORD attr = GetFileAttributesA(cbuf);
	if (attr == INVALID_FILE_ATTRIBUTES) return 0;
	return (attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

int64_t pk_file_mtime(pk_str path) {
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	struct __stat64 st;
	if (_stat64(cbuf, &st) != 0) return -1;
	return (int64_t)st.st_mtime;
}

int64_t pk_file_atime(pk_str path) {
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	struct __stat64 st;
	if (_stat64(cbuf, &st) != 0) return -1;
	return (int64_t)st.st_atime;
}

/* ========================================================================
 * Symlinks
 * ======================================================================== */

file_ReadLinkResult pk_file_readlink(pk_str path, pk_slice_u8 buf) {
	file_ReadLinkResult r;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));

	/* Open the reparse point */
	HANDLE hFile = CreateFileA(cbuf, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING,
		FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		r.tag = 1; r.val._1 = 0;
		return r;
	}

	/* Read reparse data */
	uint8_t rdbuf[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
	DWORD bytesReturned = 0;
	BOOL ok = DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, NULL, 0,
		rdbuf, sizeof(rdbuf), &bytesReturned, NULL);
	CloseHandle(hFile);

	if (!ok) {
		r.tag = 1; r.val._1 = 0;
		return r;
	}

	/* Parse the REPARSE_DATA_BUFFER to get the substitute name */
	/* Layout: ReparseTag(4) + ReparseDataLength(2) + Reserved(2) + data */
	DWORD tag_val = *(DWORD*)rdbuf;
	if (tag_val != IO_REPARSE_TAG_SYMLINK && tag_val != IO_REPARSE_TAG_MOUNT_POINT) {
		r.tag = 1; r.val._1 = 0;
		return r;
	}

	WCHAR* targetW = NULL;
	USHORT targetLen = 0;

	if (tag_val == IO_REPARSE_TAG_SYMLINK) {
		/* SymbolicLinkReparseBuffer offset: 8 bytes header + SubstituteNameOffset(2) + SubstituteNameLength(2) + PrintNameOffset(2) + PrintNameLength(2) + Flags(4) + PathBuffer */
		USHORT subOff = *(USHORT*)(rdbuf + 8);
		USHORT subLen = *(USHORT*)(rdbuf + 10);
		USHORT printOff = *(USHORT*)(rdbuf + 12);
		USHORT printLen = *(USHORT*)(rdbuf + 14);
		/* Use PrintName if available, else SubstituteName */
		if (printLen > 0) {
			targetW = (WCHAR*)(rdbuf + 20 + printOff);
			targetLen = printLen;
		} else {
			targetW = (WCHAR*)(rdbuf + 20 + subOff);
			targetLen = subLen;
		}
	} else {
		/* MountPointReparseBuffer: 8 bytes header + SubstituteNameOffset(2) + SubstituteNameLength(2) + PrintNameOffset(2) + PrintNameLength(2) + PathBuffer */
		USHORT printOff = *(USHORT*)(rdbuf + 12);
		USHORT printLen = *(USHORT*)(rdbuf + 14);
		USHORT subOff = *(USHORT*)(rdbuf + 8);
		USHORT subLen = *(USHORT*)(rdbuf + 10);
		if (printLen > 0) {
			targetW = (WCHAR*)(rdbuf + 16 + printOff);
			targetLen = printLen;
		} else {
			targetW = (WCHAR*)(rdbuf + 16 + subOff);
			targetLen = subLen;
		}
	}

	/* Convert wide chars to narrow (ASCII subset) */
	size_t nchars = targetLen / sizeof(WCHAR);
	size_t copylen = nchars < buf.len ? nchars : buf.len;
	for (size_t i = 0; i < copylen; i++) {
		buf.data[i] = (uint8_t)(targetW[i] & 0xFF);
	}

	r.tag = 0;
	/* Write str fields through the union — in single-TU mode _0 is pk_str,
	   in separate-TU mode we write data+len at the correct offsets */
	{
		pk_str* sp = (pk_str*)&r.val._0;
		sp->data = buf.data;
		sp->len = copylen;
	}
	return r;
}

file_VoidResult pk_file_makelink(pk_str target, pk_str path) {
	file_VoidResult r;
	char tbuf[4096], pbuf[4096];
	pk_str_to_cstr(target, tbuf, sizeof(tbuf));
	pk_str_to_cstr(path, pbuf, sizeof(pbuf));

	/* Determine if target is a directory */
	DWORD attr = GetFileAttributesA(tbuf);
	DWORD flags = 0;
	if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
		flags = SYMBOLIC_LINK_FLAG_DIRECTORY;

	/* SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE = 0x2 (Win10 1703+ Developer Mode) */
	flags |= 0x2;

	if (CreateSymbolicLinkA(pbuf, tbuf, flags)) {
		r.tag = 0; r.val._0 = 0;
	} else {
		/* Retry without unprivileged flag in case OS doesn't support it */
		flags &= ~0x2u;
		if (CreateSymbolicLinkA(pbuf, tbuf, flags)) {
			r.tag = 0; r.val._0 = 0;
		} else {
			r.tag = 1; r.val._1 = 0;
		}
	}
	return r;
}

/* ========================================================================
 * Directory listing
 * ======================================================================== */

file_DirOpenResult pk_file_opendir(pk_str path) {
	file_DirOpenResult r;
	int slot = -1;
	for (int i = 0; i < PK_MAX_OPEN_DIRS; i++) {
		if (!pk_dirs_active[i]) { slot = i; break; }
	}
	if (slot < 0) { r.tag = 1; r.val._1 = 0; return r; }

	/* Build search pattern "path\*" */
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	size_t plen = 0;
	while (cbuf[plen] != '\0') plen++;
	/* Append \* if there's room */
	if (plen + 3 < sizeof(cbuf)) {
		if (plen > 0 && cbuf[plen - 1] != '\\' && cbuf[plen - 1] != '/') {
			cbuf[plen++] = '\\';
		}
		cbuf[plen++] = '*';
		cbuf[plen] = '\0';
	}

	HANDLE h = FindFirstFileA(cbuf, &pk_dirs[slot].findData);
	if (h == INVALID_HANDLE_VALUE) {
		r.tag = 1; r.val._1 = 0;
		return r;
	}

	pk_dirs[slot].findHandle = h;
	pk_dirs[slot].first = 1;
	pk_dirs_active[slot] = 1;

	r.tag = 0; r.val._0 = (int64_t)slot;
	return r;
}

file_DirEntry pk_file_dirnext(int64_t handle, pk_slice_u8 buf) {
	file_DirEntry r;
	int slot = (int)handle;
	if (slot < 0 || slot >= PK_MAX_OPEN_DIRS || !pk_dirs_active[slot]) {
		r.tag = 1; r.val._1 = 0;
		return r;
	}
	pk_dir_state* ds = &pk_dirs[slot];

	for (;;) {
		if (ds->first) {
			ds->first = 0;
			/* findData already contains the first entry from FindFirstFileA */
		} else {
			if (!FindNextFileA(ds->findHandle, &ds->findData)) {
				/* No more entries */
				r.tag = 2; r.val._2 = 0;
				return r;
			}
		}

		char* d_name = ds->findData.cFileName;
		if (pk_file_is_dot_or_dotdot(d_name)) continue;

		/* Determine file type for dir cache */
		DWORD attr = ds->findData.dwFileAttributes;
		uint8_t dtype = PK_WIN_DT_UNKNOWN;
		if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
			dtype = PK_WIN_DT_LNK;
		} else if (attr & FILE_ATTRIBUTE_DIRECTORY) {
			dtype = PK_WIN_DT_DIR;
		} else {
			dtype = PK_WIN_DT_REG;
		}

		return pk_file_make_dir_entry(buf, d_name, dtype);
	}
}

void pk_file_closedir(int64_t handle) {
	int slot = (int)handle;
	if (slot < 0 || slot >= PK_MAX_OPEN_DIRS || !pk_dirs_active[slot]) return;
	FindClose(pk_dirs[slot].findHandle);
	pk_dirs_active[slot] = 0;
}
