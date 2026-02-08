/* pk_fmt.c — Formatted output for aarch64 Linux raw syscalls */

void pk_fmt_print(fmt_Buffer* b) {
	if (b->index > 0) {
		pk_syscall3(__PK_SYS_write, 1, (int64_t)b->buf.data, (int64_t)b->index);
	}
}

void pk_fmt_println(fmt_Buffer* b) {
	if (b->index > 0) {
		pk_syscall3(__PK_SYS_write, 1, (int64_t)b->buf.data, (int64_t)b->index);
	}
	pk_syscall3(__PK_SYS_write, 1, (int64_t)"\n", 1);
}

void pk_fmt_eprint(fmt_Buffer* b) {
	if (b->index > 0) {
		pk_syscall3(__PK_SYS_write, 2, (int64_t)b->buf.data, (int64_t)b->index);
	}
}

void pk_fmt_prints(pk_str s) {
	if (s.len > 0) {
		pk_syscall3(__PK_SYS_write, 1, (int64_t)s.data, (int64_t)s.len);
	}
}

void pk_fmt_printsln(pk_str s) {
	if (s.len > 0) {
		pk_syscall3(__PK_SYS_write, 1, (int64_t)s.data, (int64_t)s.len);
	}
	pk_syscall3(__PK_SYS_write, 1, (int64_t)"\n", 1);
}
