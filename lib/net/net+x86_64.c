/* pk_net.c — Networking for x86_64 Linux raw syscalls */

static void pk_net_make_addr(pk_str addr, uint16_t port, struct pk_sockaddr_in* sa) {
	sa->family = PK_AF_INET;
	sa->port = pk_htons(port);
	sa->addr = pk_parse_ipv4(addr);
	for (int i = 0; i < 8; i++) sa->zero[i] = 0;
}

static int32_t pk_net_setsockopt_reuse(int32_t fd) {
	int32_t optval = 1;
	return (int32_t)pk_syscall5(__PK_SYS_setsockopt,
		fd, PK_SOL_SOCKET, PK_SO_REUSEADDR,
		(int64_t)&optval, (int64_t)sizeof(optval));
}

/* TCP */

net_ListenResult pk_net_listen(pk_str addr, uint16_t port, int32_t backlog) {
	net_ListenResult r;
	int64_t fd = pk_syscall3(__PK_SYS_socket, PK_AF_INET, PK_SOCK_STREAM, PK_IPPROTO_TCP);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; return r; }
	pk_net_setsockopt_reuse((int32_t)fd);
	struct pk_sockaddr_in sa;
	pk_net_make_addr(addr, port, &sa);
	if (pk_syscall3(__PK_SYS_bind, (int32_t)fd, (int64_t)&sa, (int64_t)sizeof(sa)) < 0) {
		pk_syscall1(__PK_SYS_close, (int32_t)fd);
		r.tag = 1; r.val._1 = 0; return r;
	}
	if (pk_syscall2(__PK_SYS_listen, (int32_t)fd, backlog) < 0) {
		pk_syscall1(__PK_SYS_close, (int32_t)fd);
		r.tag = 1; r.val._1 = 0; return r;
	}
	r.tag = 0; r.val._0 = (int32_t)fd;
	return r;
}

net_AcceptResult pk_net_accept(int32_t fd) {
	net_AcceptResult r;
	int64_t ret = pk_syscall3(__PK_SYS_accept, fd, 0, 0);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int32_t)ret; }
	return r;
}

net_ConnectResult pk_net_connect(pk_str addr, uint16_t port) {
	net_ConnectResult r;
	int64_t fd = pk_syscall3(__PK_SYS_socket, PK_AF_INET, PK_SOCK_STREAM, PK_IPPROTO_TCP);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; return r; }
	struct pk_sockaddr_in sa;
	pk_net_make_addr(addr, port, &sa);
	if (pk_syscall3(__PK_SYS_connect, (int32_t)fd, (int64_t)&sa, (int64_t)sizeof(sa)) < 0) {
		pk_syscall1(__PK_SYS_close, (int32_t)fd);
		r.tag = 1; r.val._1 = 0; return r;
	}
	r.tag = 0; r.val._0 = (int32_t)fd;
	return r;
}

net_SendResult pk_net_send(int32_t fd, pk_slice_u8 data) {
	net_SendResult r;
	int64_t ret = pk_syscall6(__PK_SYS_sendto, fd, (int64_t)data.data, (int64_t)data.len, 0, 0, 0);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = ret; }
	return r;
}

net_RecvResult pk_net_recv(int32_t fd, pk_slice_u8 buf) {
	net_RecvResult r;
	int64_t ret = pk_syscall6(__PK_SYS_recvfrom, fd, (int64_t)buf.data, (int64_t)buf.len, 0, 0, 0);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = ret; }
	return r;
}

void pk_net_close(int32_t fd) {
	pk_syscall1(__PK_SYS_close, fd);
}

/* UDP */

net_ListenResult pk_net_bind_udp(pk_str addr, uint16_t port) {
	net_ListenResult r;
	int64_t fd = pk_syscall3(__PK_SYS_socket, PK_AF_INET, PK_SOCK_DGRAM, PK_IPPROTO_UDP);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; return r; }
	pk_net_setsockopt_reuse((int32_t)fd);
	struct pk_sockaddr_in sa;
	pk_net_make_addr(addr, port, &sa);
	if (pk_syscall3(__PK_SYS_bind, (int32_t)fd, (int64_t)&sa, (int64_t)sizeof(sa)) < 0) {
		pk_syscall1(__PK_SYS_close, (int32_t)fd);
		r.tag = 1; r.val._1 = 0; return r;
	}
	r.tag = 0; r.val._0 = (int32_t)fd;
	return r;
}

net_SendResult pk_net_send_udp(int32_t fd, pk_slice_u8 data, pk_str addr, uint16_t port) {
	net_SendResult r;
	struct pk_sockaddr_in sa;
	pk_net_make_addr(addr, port, &sa);
	int64_t ret = pk_syscall6(__PK_SYS_sendto, fd,
		(int64_t)data.data, (int64_t)data.len, 0,
		(int64_t)&sa, (int64_t)sizeof(sa));
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = ret; }
	return r;
}

net_RecvResult pk_net_recv_udp(int32_t fd, pk_slice_u8 buf, pk_slice_u8 addr_buf, uint16_t* port_out) {
	net_RecvResult r;
	struct pk_sockaddr_in sa;
	for (int i = 0; i < (int)sizeof(sa); i++) ((uint8_t*)&sa)[i] = 0;
	uint32_t salen = (uint32_t)sizeof(sa);

	int64_t ret = pk_syscall6(__PK_SYS_recvfrom, fd,
		(int64_t)buf.data, (int64_t)buf.len, 0,
		(int64_t)&sa, (int64_t)&salen);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; return r; }

	if (addr_buf.data && addr_buf.len > 0) {
		uint32_t ip = pk_htonl(sa.addr);
		uint8_t a = (uint8_t)(ip >> 24), b = (uint8_t)(ip >> 16);
		uint8_t c = (uint8_t)(ip >> 8), d = (uint8_t)(ip);
		size_t pos = 0;
		uint8_t parts[4] = {a, b, c, d};
		for (int p = 0; p < 4 && pos < addr_buf.len; p++) {
			if (p > 0 && pos < addr_buf.len) addr_buf.data[pos++] = '.';
			uint8_t v = parts[p];
			if (v >= 100 && pos < addr_buf.len) { addr_buf.data[pos++] = (uint8_t)('0' + v / 100); v %= 100; }
			if ((parts[p] >= 10) && pos < addr_buf.len) { addr_buf.data[pos++] = (uint8_t)('0' + v / 10); v %= 10; }
			if (pos < addr_buf.len) addr_buf.data[pos++] = (uint8_t)('0' + v);
		}
		if (pos < addr_buf.len) addr_buf.data[pos] = 0;
	}
	if (port_out) *port_out = pk_htons(sa.port);

	r.tag = 0; r.val._0 = ret;
	return r;
}

/* Multicast */

net_VoidResult pk_net_listen_mc(int32_t fd, pk_str group) {
	net_VoidResult r;
	struct pk_ip_mreq mreq;
	mreq.multiaddr = pk_parse_ipv4(group);
	mreq.interface = 0;
	int64_t ret = pk_syscall5(__PK_SYS_setsockopt, fd,
		PK_IPPROTO_IP, PK_IP_ADD_MEMBERSHIP,
		(int64_t)&mreq, (int64_t)sizeof(mreq));
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

net_VoidResult pk_net_leave_mc(int32_t fd, pk_str group) {
	net_VoidResult r;
	struct pk_ip_mreq mreq;
	mreq.multiaddr = pk_parse_ipv4(group);
	mreq.interface = 0;
	int64_t ret = pk_syscall5(__PK_SYS_setsockopt, fd,
		PK_IPPROTO_IP, PK_IP_DROP_MEMBERSHIP,
		(int64_t)&mreq, (int64_t)sizeof(mreq));
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}
