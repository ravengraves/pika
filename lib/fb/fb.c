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

/* Compatibility wrappers for legacy/generated call sites that still reference
 * module-qualified fb_* symbols instead of the pk_fb_* C backing names. */
void fb_Print(fb_Buffer* b) { pk_fb_print(b); }
void fb_Println(fb_Buffer* b) { pk_fb_println(b); }
void fb_EPrint(fb_Buffer* b) { pk_fb_eprint(b); }
void fb_Prints(pk_str s) { pk_fb_prints(s); }
void fb_Printsln(pk_str s) { pk_fb_printsln(s); }
