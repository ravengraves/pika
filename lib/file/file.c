/* pk_file.c — backend-agnostic file helpers */

int pk_file_dir_cache_match(pk_str path) {
	if (pk__dir_last_name_len == 0) return 0;
	if (path.len != pk__dir_last_name_len) return 0;
	for (size_t i = 0; i < path.len; i++) {
		if (path.data[i] != (uint8_t)pk__dir_last_name[i]) return 0;
	}
	return 1;
}

int pk_file_is_dot_or_dotdot(const char* d_name) {
	if (d_name[0] != '.') return 0;
	if (d_name[1] == '\0') return 1;
	if (d_name[1] == '.' && d_name[2] == '\0') return 1;
	return 0;
}

file_DirEntry pk_file_make_dir_entry(pk_slice_u8 buf, const char* d_name, uint8_t d_type) {
	file_DirEntry r;

	size_t namelen = 0;
	while (d_name[namelen] != '\0') namelen++;

	size_t copylen = namelen < buf.len ? namelen : buf.len;
	for (size_t i = 0; i < copylen; i++) buf.data[i] = (uint8_t)d_name[i];

	size_t max_cache = sizeof(pk__dir_last_name) - 1;
	size_t cachelen = namelen < max_cache ? namelen : max_cache;
	for (size_t i = 0; i < cachelen; i++) pk__dir_last_name[i] = d_name[i];
	pk__dir_last_name[cachelen] = '\0';
	pk__dir_last_name_len = cachelen;
	pk__dir_last_type = d_type;

	r.tag = 0;
	r.val._0.data = buf.data;
	r.val._0.len = copylen;
	return r;
}
