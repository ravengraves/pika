/* pk_fmt.c — Formatted output for Windows (MinGW) */

void pk_fmt_print(fmt_Buffer* b) {
	if (b->index > 0) {
		_write(1, b->buf.data, (unsigned int)b->index);
	}
}

void pk_fmt_println(fmt_Buffer* b) {
	if (b->index > 0) {
		_write(1, b->buf.data, (unsigned int)b->index);
	}
	_write(1, "\n", 1);
}

void pk_fmt_eprint(fmt_Buffer* b) {
	if (b->index > 0) {
		_write(2, b->buf.data, (unsigned int)b->index);
	}
}

void pk_fmt_prints(pk_str s) {
	if (s.len > 0) {
		_write(1, s.data, (unsigned int)s.len);
	}
}

void pk_fmt_printsln(pk_str s) {
	if (s.len > 0) {
		_write(1, s.data, (unsigned int)s.len);
	}
	_write(1, "\n", 1);
}
