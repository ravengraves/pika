/* pk_io.c — I/O polling for Windows (MinGW) */

io_PollResult pk_io_poll(int32_t fd, int32_t timeout, int16_t watch) {
	io_PollResult r;
	pk__ensure_winsock();

	WSAPOLLFD pfd;
	pfd.fd = (SOCKET)(intptr_t)fd;
	pfd.events = watch;
	pfd.revents = 0;

	int ret = WSAPoll(&pfd, 1, timeout);
	if (ret < 0) { r.tag = 2; r.val._2 = 0; }
	else if (ret == 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = pfd.revents; }
	return r;
}

io_PollManyResult pk_io_pollmany(pk_slice_io_PollFd fds, int32_t timeout) {
	io_PollManyResult r;
	pk__ensure_winsock();

	/* Build WSAPOLLFD array on stack (max reasonable count) */
	size_t nfds = fds.len;
	if (nfds == 0) { r.tag = 1; r.val._1 = 0; return r; }

	/* Use stack for small counts, otherwise cap at 256 */
	WSAPOLLFD wfds[256];
	size_t count = nfds < 256 ? nfds : 256;

	for (size_t i = 0; i < count; i++) {
		wfds[i].fd = (SOCKET)(intptr_t)fds.data[i].fd;
		wfds[i].events = fds.data[i].events;
		wfds[i].revents = 0;
	}

	int ret = WSAPoll(wfds, (ULONG)count, timeout);

	/* Copy revents back */
	for (size_t i = 0; i < count; i++) {
		fds.data[i].revents = wfds[i].revents;
	}

	if (ret < 0) { r.tag = 2; r.val._2 = 0; }
	else if (ret == 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int32_t)ret; }
	return r;
}

io_InputResult pk_io_input(pk_slice_u8 buf, pk_str prompt, int32_t timeout) {
	io_InputResult r;
	/* Write prompt to stderr */
	if (prompt.len > 0) {
		_write(2, prompt.data, (unsigned int)prompt.len);
	}
	/* Poll stdin with timeout */
	if (timeout >= 0) {
		HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
		DWORD wait = WaitForSingleObject(hStdin, (DWORD)timeout);
		if (wait == WAIT_TIMEOUT) { r.tag = 1; r.val._1 = 0; return r; }
		if (wait == WAIT_FAILED) { r.tag = 2; r.val._2 = 0; return r; }
	}
	/* Read from stdin */
	int n = _read(0, buf.data, (unsigned int)buf.len);
	if (n < 0) { r.tag = 2; r.val._2 = 0; return r; }
	return pk_io_input_ok(buf, pk_io_trim_line_end(buf, (size_t)n));
}
