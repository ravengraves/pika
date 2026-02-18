/* pk_file.c — File operations using libc (POSIX) */

/* ========================================================================
 * Directory state pool
 * ======================================================================== */
#define PK_MAX_OPEN_DIRS 8

typedef struct {
	DIR* dp;
} pk_dir_state;

static pk_dir_state pk_dirs[PK_MAX_OPEN_DIRS];
static uint8_t pk_dirs_active[PK_MAX_OPEN_DIRS];

/* ========================================================================
 * Internal helpers
 * ======================================================================== */

static int32_t pk_mode_to_flags(pk_str mode) {
	if (pk_str_eq_lit(mode, "r", 1))
		return O_RDONLY;
	if (pk_str_eq_lit(mode, "w", 1))
		return O_WRONLY | O_CREAT | O_TRUNC;
	if (pk_str_eq_lit(mode, "a", 1))
		return O_WRONLY | O_CREAT | O_APPEND;
	if (pk_str_eq_lit(mode, "rw", 2))
		return O_RDWR | O_CREAT;
	if (pk_str_eq_lit(mode, "rw+", 3))
		return O_RDWR | O_CREAT | O_TRUNC;
	return -1;
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
	int fd = open(cbuf, flags, 0644);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int32_t)fd; }
	return r;
}

void pk_file_close(int32_t fd) {
	close(fd);
}

file_ReadResult pk_file_read(int32_t fd, pk_slice_u8 buf) {
	file_ReadResult r;
	ssize_t ret = read(fd, buf.data, buf.len);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int64_t)ret; }
	return r;
}

file_WriteResult pk_file_write(int32_t fd, pk_slice_u8 data) {
	file_WriteResult r;
	ssize_t ret = write(fd, data.data, data.len);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int64_t)ret; }
	return r;
}

file_SeekResult pk_file_seek(int32_t fd, int64_t pos) {
	file_SeekResult r;
	off_t ret = lseek(fd, (off_t)pos, SEEK_SET);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int64_t)ret; }
	return r;
}

int64_t pk_file_loc(int32_t fd) {
	return (int64_t)lseek(fd, 0, SEEK_CUR);
}

/* ========================================================================
 * Filesystem operations
 * ======================================================================== */

file_VoidResult pk_file_del(pk_str path) {
	file_VoidResult r;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	int ret = unlink(cbuf);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

file_VoidResult pk_file_rename(pk_str old_path, pk_str new_path) {
	file_VoidResult r;
	char cbuf1[4096], cbuf2[4096];
	pk_str_to_cstr(old_path, cbuf1, sizeof(cbuf1));
	pk_str_to_cstr(new_path, cbuf2, sizeof(cbuf2));
	int ret = rename(cbuf1, cbuf2);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

file_VoidResult pk_file_mkdir(pk_str path) {
	file_VoidResult r;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	int ret = mkdir(cbuf, 0755);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

file_VoidResult pk_file_rmdir(pk_str path) {
	file_VoidResult r;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	int ret = rmdir(cbuf);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

/* ========================================================================
 * Stat-based queries
 * ======================================================================== */

int64_t pk_file_size(pk_str path) {
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	struct stat st;
	if (stat(cbuf, &st) < 0) return -1;
	return (int64_t)st.st_size;
}

_Bool pk_file_exists(pk_str path) {
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	struct stat st;
	return stat(cbuf, &st) == 0;
}

_Bool pk_file_is_dir(pk_str path) {
	if (pk_file_dir_cache_match(path) && pk__dir_last_type == DT_DIR) return 1;
	if (pk_file_dir_cache_match(path) && pk__dir_last_type != DT_UNKNOWN) return 0;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	struct stat st;
	if (stat(cbuf, &st) < 0) return 0;
	return S_ISDIR(st.st_mode);
}

_Bool pk_file_is_file(pk_str path) {
	if (pk_file_dir_cache_match(path) && pk__dir_last_type == DT_REG) return 1;
	if (pk_file_dir_cache_match(path) && pk__dir_last_type != DT_UNKNOWN) return 0;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	struct stat st;
	if (stat(cbuf, &st) < 0) return 0;
	return S_ISREG(st.st_mode);
}

_Bool pk_file_is_link(pk_str path) {
	if (pk_file_dir_cache_match(path) && pk__dir_last_type == DT_LNK) return 1;
	if (pk_file_dir_cache_match(path) && pk__dir_last_type != DT_UNKNOWN) return 0;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	struct stat st;
	if (lstat(cbuf, &st) < 0) return 0;
	return S_ISLNK(st.st_mode);
}

int64_t pk_file_mtime(pk_str path) {
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	struct stat st;
	if (stat(cbuf, &st) < 0) return -1;
	return (int64_t)st.st_mtime;
}

int64_t pk_file_atime(pk_str path) {
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	struct stat st;
	if (stat(cbuf, &st) < 0) return -1;
	return (int64_t)st.st_atime;
}

/* ========================================================================
 * Symlinks
 * ======================================================================== */

file_ReadLinkResult pk_file_readlink(pk_str path, pk_slice_u8 buf) {
	file_ReadLinkResult r;
	char cbuf[4096];
	pk_str_to_cstr(path, cbuf, sizeof(cbuf));
	ssize_t ret = readlink(cbuf, (char*)buf.data, buf.len);
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
	int ret = symlink(tbuf, pbuf);
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
	DIR* dp = opendir(cbuf);
	if (!dp) { r.tag = 1; r.val._1 = 0; return r; }
	pk_dirs[slot].dp = dp;
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

	for (;;) {
		errno = 0;
		struct dirent* ent = readdir(pk_dirs[slot].dp);
		if (!ent) {
			if (errno != 0) { r.tag = 1; r.val._1 = 0; }
			else { r.tag = 2; r.val._2 = 0; }
			return r;
		}

		char* d_name = ent->d_name;

		if (pk_file_is_dot_or_dotdot(d_name)) continue;
		return pk_file_make_dir_entry(buf, d_name, ent->d_type);
	}
}

void pk_file_closedir(int64_t handle) {
	int slot = (int)handle;
	if (slot < 0 || slot >= PK_MAX_OPEN_DIRS || !pk_dirs_active[slot]) return;
	closedir(pk_dirs[slot].dp);
	pk_dirs_active[slot] = 0;
}
