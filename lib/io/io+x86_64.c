/* pk_io.c — I/O polling for x86_64 Linux raw syscalls */

io_PollResult pk_io_poll(int32_t fd, int32_t timeout, int16_t watch) {
	io_PollResult r;
	struct pk_pollfd pfd;
	pfd.fd = fd;
	pfd.events = watch;
	pfd.revents = 0;
	int64_t ret = pk_syscall3(__PK_SYS_poll, (int64_t)&pfd, 1, timeout);
	if (ret < 0) { r.tag = 2; r.val._2 = 0; }
	else if (ret == 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = pfd.revents; }
	return r;
}

io_PollManyResult pk_io_pollmany(pk_slice_io_PollFd fds, int32_t timeout) {
	io_PollManyResult r;
	int64_t ret = pk_syscall3(__PK_SYS_poll, (int64_t)fds.data, (int64_t)fds.len, timeout);
	if (ret < 0) { r.tag = 2; r.val._2 = 0; }
	else if (ret == 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int32_t)ret; }
	return r;
}

io_InputResult pk_io_input(pk_slice_u8 buf, pk_str prompt, int32_t timeout) {
	io_InputResult r;
	/* Write prompt to stderr */
	if (prompt.len > 0) {
		pk_syscall3(__PK_SYS_write, 2, (int64_t)prompt.data, (int64_t)prompt.len);
	}
	/* Poll stdin with timeout */
	if (timeout >= 0) {
		struct pk_pollfd pfd;
		pfd.fd = 0;
		pfd.events = 1; /* POLLIN */
		pfd.revents = 0;
		int64_t pret = pk_syscall3(__PK_SYS_poll, (int64_t)&pfd, 1, timeout);
		if (pret == 0) { r.tag = 1; r.val._1 = 0; return r; }
		if (pret < 0) { r.tag = 2; r.val._2 = 0; return r; }
	}
	/* Read from stdin */
	int64_t n = pk_syscall3(__PK_SYS_read, 0, (int64_t)buf.data, (int64_t)buf.len);
	if (n < 0) { r.tag = 2; r.val._2 = 0; return r; }
	return pk_io_input_ok(buf, pk_io_trim_line_end(buf, (size_t)n));
}
