/* pk_fb.c — Windows write primitive for shared formatter logic */

void pk_fb_write_fd(int32_t fd, const uint8_t* data, size_t len) {
	if (len > 0) {
		_write(fd, data, (unsigned int)len);
	}
}
