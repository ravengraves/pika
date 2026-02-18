/* pk_io.c — backend-agnostic I/O helpers */

size_t pk_io_trim_line_end(pk_slice_u8 buf, size_t len) {
	if (len > 0 && buf.data[len - 1] == '\n') len--;
	if (len > 0 && buf.data[len - 1] == '\r') len--;
	return len;
}

io_InputResult pk_io_input_ok(pk_slice_u8 buf, size_t len) {
	io_InputResult r;
	r.tag = 0;
	r.val._0.data = buf.data;
	r.val._0.len = len;
	return r;
}
