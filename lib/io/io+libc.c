/* pk_io.c — I/O polling using libc (POSIX) */

io_PollResult pk_io_poll(int32_t fd, int32_t timeout, int16_t watch) {
	io_PollResult r;
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = watch;
	pfd.revents = 0;
	int ret = poll(&pfd, 1, timeout);
	if (ret < 0) { r.tag = 2; r.val._2 = 0; }
	else if (ret == 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = pfd.revents; }
	return r;
}

io_PollManyResult pk_io_pollmany(pk_slice_io_PollFd fds, int32_t timeout) {
	io_PollManyResult r;
	/* io_PollFd has the same layout as struct pollfd (fd:i32, events:i16, revents:i16) */
	int ret = poll((struct pollfd*)fds.data, (nfds_t)fds.len, timeout);
	if (ret < 0) { r.tag = 2; r.val._2 = 0; }
	else if (ret == 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int32_t)ret; }
	return r;
}

io_InputResult pk_io_input(pk_slice_u8 buf, pk_str prompt, int32_t timeout) {
	io_InputResult r;
	/* Write prompt to stderr */
	if (prompt.len > 0) {
		(void)write(STDERR_FILENO, prompt.data, prompt.len);
	}
	/* Poll stdin with timeout */
	if (timeout >= 0) {
		struct pollfd pfd;
		pfd.fd = 0;
		pfd.events = POLLIN;
		pfd.revents = 0;
		int pret = poll(&pfd, 1, timeout);
		if (pret == 0) { r.tag = 1; r.val._1 = 0; return r; }
		if (pret < 0) { r.tag = 2; r.val._2 = 0; return r; }
	}
	/* Read from stdin */
	ssize_t n = read(STDIN_FILENO, buf.data, buf.len);
	if (n < 0) { r.tag = 2; r.val._2 = 0; return r; }
	return pk_io_input_ok(buf, pk_io_trim_line_end(buf, (size_t)n));
}
