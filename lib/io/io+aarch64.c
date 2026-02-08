/* pk_io.c — I/O polling for aarch64 Linux raw syscalls */
/* aarch64 has no poll; use ppoll(fds, nfds, timeout_ts, sigmask, sigsetsize) */

io_PollResult pk_io_poll(int32_t fd, int32_t timeout, int16_t watch) {
	io_PollResult r;
	struct pk_pollfd pfd;
	pfd.fd = fd;
	pfd.events = watch;
	pfd.revents = 0;

	struct pk_timespec ts;
	struct pk_timespec* tsp = (void*)0;
	if (timeout >= 0) {
		ts.sec = timeout / 1000;
		ts.nsec = (int64_t)(timeout % 1000) * 1000000LL;
		tsp = &ts;
	}

	int64_t ret = pk_syscall5(__PK_SYS_ppoll, (int64_t)&pfd, 1, (int64_t)tsp, 0, 0);
	if (ret < 0) { r.tag = 2; r.val._2 = 0; }
	else if (ret == 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = pfd.revents; }
	return r;
}

io_PollManyResult pk_io_pollmany(pk_slice_io_PollFd fds, int32_t timeout) {
	io_PollManyResult r;

	struct pk_timespec ts;
	struct pk_timespec* tsp = (void*)0;
	if (timeout >= 0) {
		ts.sec = timeout / 1000;
		ts.nsec = (int64_t)(timeout % 1000) * 1000000LL;
		tsp = &ts;
	}

	int64_t ret = pk_syscall5(__PK_SYS_ppoll, (int64_t)fds.data, (int64_t)fds.len, (int64_t)tsp, 0, 0);
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
	/* Poll stdin with timeout (ppoll on aarch64) */
	if (timeout >= 0) {
		struct pk_pollfd pfd;
		pfd.fd = 0;
		pfd.events = 1; /* POLLIN */
		pfd.revents = 0;
		struct pk_timespec ts;
		ts.sec = timeout / 1000;
		ts.nsec = (int64_t)(timeout % 1000) * 1000000LL;
		int64_t pret = pk_syscall5(__PK_SYS_ppoll, (int64_t)&pfd, 1, (int64_t)&ts, 0, 0);
		if (pret == 0) { r.tag = 1; r.val._1 = 0; return r; }
		if (pret < 0) { r.tag = 2; r.val._2 = 0; return r; }
	}
	/* Read from stdin */
	int64_t n = pk_syscall3(__PK_SYS_read, 0, (int64_t)buf.data, (int64_t)buf.len);
	if (n < 0) { r.tag = 2; r.val._2 = 0; return r; }
	/* Strip trailing newline */
	size_t len = (size_t)n;
	if (len > 0 && buf.data[len - 1] == '\n') len--;
	if (len > 0 && buf.data[len - 1] == '\r') len--;
	r.tag = 0;
	r.val._0.data = buf.data;
	r.val._0.len = len;
	return r;
}
