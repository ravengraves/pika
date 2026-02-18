/* pk_file.c — File operations for x86_64 Linux raw syscalls */

/* ========================================================================
 * Directory state pool
 * ======================================================================== */
#define PK_MAX_OPEN_DIRS 8

typedef struct {
	int32_t fd;
	uint8_t buf[4096];
	size_t pos, len;
} pk_dir_state;

static pk_dir_state pk_dirs[PK_MAX_OPEN_DIRS];
static uint8_t pk_dirs_active[PK_MAX_OPEN_DIRS];

/* ========================================================================
 * Internal helpers
 * ======================================================================== */

static int32_t pk_mode_to_flags(pk_str mode) {
	if (pk_str_eq_lit(mode, "r", 1))
		return PK_O_RDONLY;
	if (pk_str_eq_lit(mode, "w", 1))
		return PK_O_WRONLY | PK_O_CREAT | PK_O_TRUNC;
	if (pk_str_eq_lit(mode, "a", 1))
		return PK_O_WRONLY | PK_O_CREAT | PK_O_APPEND;
	if (pk_str_eq_lit(mode, "rw", 2))
		return PK_O_RDWR | PK_O_CREAT;
	if (pk_str_eq_lit(mode, "rw+", 3))
		return PK_O_RDWR | PK_O_CREAT | PK_O_TRUNC;
	return -1;
}

static int32_t pk_do_stat(pk_str path, struct pk_linux_stat* st, int32_t flags) {
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	return (int32_t)pk_syscall4(__PK_SYS_fstatat, PK_AT_FDCWD, (int64_t)cbuf, (int64_t)st, flags);
}

/* ========================================================================
 * Core I/O
 * ======================================================================== */

file_OpenResult pk_file_open(pk_str path, pk_str mode) {
	file_OpenResult r;
	int32_t flags = pk_mode_to_flags(mode);
	if (flags < 0) { r.tag = 1; r.val._1 = 0; return r; }
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	int64_t ret = pk_syscall4(__PK_SYS_openat, PK_AT_FDCWD, (int64_t)cbuf, flags, 0644);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int32_t)ret; }
	return r;
}

void pk_file_close(int32_t fd) {
	pk_syscall1(__PK_SYS_close, fd);
}

file_ReadResult pk_file_read(int32_t fd, pk_slice_u8 buf) {
	file_ReadResult r;
	int64_t ret = pk_syscall3(__PK_SYS_read, fd, (int64_t)buf.data, (int64_t)buf.len);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = ret; }
	return r;
}

file_WriteResult pk_file_write(int32_t fd, pk_slice_u8 data) {
	file_WriteResult r;
	int64_t ret = pk_syscall3(__PK_SYS_write, fd, (int64_t)data.data, (int64_t)data.len);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = ret; }
	return r;
}

file_SeekResult pk_file_seek(int32_t fd, int64_t pos) {
	file_SeekResult r;
	int64_t ret = pk_syscall3(__PK_SYS_lseek, fd, pos, PK_SEEK_SET);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = ret; }
	return r;
}

int64_t pk_file_loc(int32_t fd) {
	return pk_syscall3(__PK_SYS_lseek, fd, 0, PK_SEEK_CUR);
}

/* ========================================================================
 * Filesystem operations
 * ======================================================================== */

file_VoidResult pk_file_del(pk_str path) {
	file_VoidResult r;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	int64_t ret = pk_syscall3(__PK_SYS_unlinkat, PK_AT_FDCWD, (int64_t)cbuf, 0);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

file_VoidResult pk_file_rename(pk_str old_path, pk_str new_path) {
	file_VoidResult r;
	char cbuf1[4096], cbuf2[4096];
	pk_str_to_cstr(old_path, cbuf1, sizeof(cbuf1));
	pk_str_to_cstr(new_path, cbuf2, sizeof(cbuf2));
	int64_t ret = pk_syscall5(__PK_SYS_renameat2,
		PK_AT_FDCWD, (int64_t)cbuf1, PK_AT_FDCWD, (int64_t)cbuf2, 0);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

file_VoidResult pk_file_mkdir(pk_str path) {
	file_VoidResult r;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	int64_t ret = pk_syscall3(__PK_SYS_mkdirat, PK_AT_FDCWD, (int64_t)cbuf, 0755);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

file_VoidResult pk_file_rmdir(pk_str path) {
	file_VoidResult r;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	int64_t ret = pk_syscall3(__PK_SYS_unlinkat, PK_AT_FDCWD, (int64_t)cbuf, PK_AT_REMOVEDIR);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

/* ========================================================================
 * Stat-based queries
 * ======================================================================== */

int64_t pk_file_size(pk_str path) {
	struct pk_linux_stat st;
	if (pk_do_stat(path, &st, 0) < 0) return -1;
	return st.st_size;
}

_Bool pk_file_exists(pk_str path) {
	struct pk_linux_stat st;
	return pk_do_stat(path, &st, 0) == 0;
}

_Bool pk_file_is_dir(pk_str path) {
	if (pk_file_dir_cache_match(path) && pk__dir_last_type == PK_DT_DIR) return 1;
	if (pk_file_dir_cache_match(path) && pk__dir_last_type != PK_DT_UNKNOWN) return 0;
	struct pk_linux_stat st;
	if (pk_do_stat(path, &st, 0) < 0) return 0;
	return (st.st_mode & PK_S_IFMT) == PK_S_IFDIR;
}

_Bool pk_file_is_file(pk_str path) {
	if (pk_file_dir_cache_match(path) && pk__dir_last_type == PK_DT_REG) return 1;
	if (pk_file_dir_cache_match(path) && pk__dir_last_type != PK_DT_UNKNOWN) return 0;
	struct pk_linux_stat st;
	if (pk_do_stat(path, &st, 0) < 0) return 0;
	return (st.st_mode & PK_S_IFMT) == PK_S_IFREG;
}

_Bool pk_file_is_link(pk_str path) {
	if (pk_file_dir_cache_match(path) && pk__dir_last_type == PK_DT_LNK) return 1;
	if (pk_file_dir_cache_match(path) && pk__dir_last_type != PK_DT_UNKNOWN) return 0;
	struct pk_linux_stat st;
	if (pk_do_stat(path, &st, PK_AT_SYMLINK_NOFOLLOW) < 0) return 0;
	return (st.st_mode & PK_S_IFMT) == PK_S_IFLNK;
}

int64_t pk_file_mtime(pk_str path) {
	struct pk_linux_stat st;
	if (pk_do_stat(path, &st, 0) < 0) return -1;
	return st.st_mtime;
}

int64_t pk_file_atime(pk_str path) {
	struct pk_linux_stat st;
	if (pk_do_stat(path, &st, 0) < 0) return -1;
	return st.st_atime;
}

/* ========================================================================
 * Symlinks
 * ======================================================================== */

file_ReadLinkResult pk_file_readlink(pk_str path, pk_slice_u8 buf) {
	file_ReadLinkResult r;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	int64_t ret = pk_syscall3(__PK_SYS_readlink, (int64_t)cbuf, (int64_t)buf.data, (int64_t)buf.len);
	if (ret < 0) {
		r.tag = 1; r.val._1 = 0;
	} else {
		r.tag = 0;
		r.val._0.data = buf.data;
		r.val._0.len = (size_t)ret;
	}
	return r;
}

file_VoidResult pk_file_makelink(pk_str target, pk_str path) {
	file_VoidResult r;
	char tbuf[4096], pbuf[4096];
	pk_str_to_cstr(target, tbuf, sizeof(tbuf));
	pk_str_to_cstr(path, pbuf, sizeof(pbuf));
	int64_t ret = pk_syscall2(__PK_SYS_symlink, (int64_t)tbuf, (int64_t)pbuf);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
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
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	int64_t fd = pk_syscall4(__PK_SYS_openat, PK_AT_FDCWD, (int64_t)cbuf,
		PK_O_RDONLY | PK_O_DIRECTORY, 0);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; return r; }
	pk_dirs[slot].fd = (int32_t)fd;
	pk_dirs[slot].pos = 0;
	pk_dirs[slot].len = 0;
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
		if (ds->pos >= ds->len) {
			int64_t ret = pk_syscall3(__PK_SYS_getdents64, ds->fd,
				(int64_t)ds->buf, (int64_t)sizeof(ds->buf));
			if (ret < 0) { r.tag = 1; r.val._1 = 0; return r; }
			if (ret == 0) { r.tag = 2; r.val._2 = 0; return r; }
			ds->len = (size_t)ret;
			ds->pos = 0;
		}

		uint8_t* ent = &ds->buf[ds->pos];
		uint16_t reclen = *(uint16_t*)(ent + PK_DIRENT64_RECLEN_OFF);
		uint8_t d_type = ent[PK_DIRENT64_TYPE_OFF];
		char* d_name = (char*)(ent + PK_DIRENT64_NAME_OFF);
		ds->pos += reclen;

		if (pk_file_is_dot_or_dotdot(d_name)) continue;
		return pk_file_make_dir_entry(buf, d_name, d_type);
	}
}

void pk_file_closedir(int64_t handle) {
	int slot = (int)handle;
	if (slot < 0 || slot >= PK_MAX_OPEN_DIRS || !pk_dirs_active[slot]) return;
	pk_syscall1(__PK_SYS_close, pk_dirs[slot].fd);
	pk_dirs_active[slot] = 0;
	pk_dirs[slot].pos = 0;
	pk_dirs[slot].len = 0;
}
