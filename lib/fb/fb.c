/* pk_fb.c — backend-agnostic formatting helpers */

void pk_fb_write_fd(int32_t fd, const uint8_t* data, size_t len);

void pk_fb_print(fb_Buffer* b) {
	if (b->index > 0) {
		pk_fb_write_fd(1, b->buf.data, b->index);
	}
}

void pk_fb_println(fb_Buffer* b) {
	if (b->index > 0) {
		pk_fb_write_fd(1, b->buf.data, b->index);
	}
	pk_fb_write_fd(1, (const uint8_t*)"\n", 1);
}

void pk_fb_eprint(fb_Buffer* b) {
	if (b->index > 0) {
		pk_fb_write_fd(2, b->buf.data, b->index);
	}
}

void pk_fb_prints(pk_str s) {
	if (s.len > 0) {
		pk_fb_write_fd(1, s.data, s.len);
	}
}

void pk_fb_printsln(pk_str s) {
	if (s.len > 0) {
		pk_fb_write_fd(1, s.data, s.len);
	}
	pk_fb_write_fd(1, (const uint8_t*)"\n", 1);
}
