/* pk_fb.c — POSIX write primitive for shared formatter logic */

void pk_fb_write_fd(int32_t fd, const uint8_t* data, size_t len) {
	if (len > 0) {
		(void)write(fd, data, len);
	}
}
