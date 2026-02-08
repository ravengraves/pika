/* pk_fmt.c — Formatted output using libc (POSIX) */

void pk_fmt_print(fmt_Buffer* b) {
	if (b->index > 0) {
		(void)write(STDOUT_FILENO, b->buf.data, b->index);
	}
}

void pk_fmt_println(fmt_Buffer* b) {
	if (b->index > 0) {
		(void)write(STDOUT_FILENO, b->buf.data, b->index);
	}
	(void)write(STDOUT_FILENO, "\n", 1);
}

void pk_fmt_eprint(fmt_Buffer* b) {
	if (b->index > 0) {
		(void)write(STDERR_FILENO, b->buf.data, b->index);
	}
}

void pk_fmt_prints(pk_str s) {
	if (s.len > 0) {
		(void)write(STDOUT_FILENO, s.data, s.len);
	}
}

void pk_fmt_printsln(pk_str s) {
	if (s.len > 0) {
		(void)write(STDOUT_FILENO, s.data, s.len);
	}
	(void)write(STDOUT_FILENO, "\n", 1);
}
