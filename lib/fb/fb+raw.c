/* pk_fb.c — Linux raw write primitive for shared formatter logic */

void pk_fb_write_fd(int32_t fd, const uint8_t* data, size_t len) {
	if (len > 0) {
		pk_syscall3(__PK_SYS_write, fd, (int64_t)data, (int64_t)len);
	}
}
