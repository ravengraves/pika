/* pk_net.c — Networking using libc (POSIX) */

static void pk_net_make_addr(pk_str addr, uint16_t port, struct sockaddr_in* sa) {
	memset(sa, 0, sizeof(*sa));
	sa->sin_family = AF_INET;
	sa->sin_port = pk_htons(port);
	sa->sin_addr.s_addr = pk_parse_ipv4(addr);
}

static int pk_net_setsockopt_reuse(int fd) {
	int optval = 1;
	return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

/* TCP */

net_ListenResult pk_net_listen(pk_str addr, uint16_t port, int32_t backlog) {
	net_ListenResult r;
	int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; return r; }
	pk_net_setsockopt_reuse(fd);
	struct sockaddr_in sa;
	pk_net_make_addr(addr, port, &sa);
	if (bind(fd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
		close(fd);
		r.tag = 1; r.val._1 = 0; return r;
	}
	if (listen(fd, backlog) < 0) {
		close(fd);
		r.tag = 1; r.val._1 = 0; return r;
	}
	r.tag = 0; r.val._0 = (int32_t)fd;
	return r;
}

net_AcceptResult pk_net_accept(int32_t fd) {
	net_AcceptResult r;
	int ret = accept(fd, NULL, NULL);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int32_t)ret; }
	return r;
}

net_ConnectResult pk_net_connect(pk_str addr, uint16_t port) {
	net_ConnectResult r;
	int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; return r; }
	struct sockaddr_in sa;
	pk_net_make_addr(addr, port, &sa);
	if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
		close(fd);
		r.tag = 1; r.val._1 = 0; return r;
	}
	r.tag = 0; r.val._0 = (int32_t)fd;
	return r;
}

net_SendResult pk_net_send(int32_t fd, pk_slice_u8 data) {
	net_SendResult r;
	ssize_t ret = send(fd, data.data, data.len, 0);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int64_t)ret; }
	return r;
}

net_RecvResult pk_net_recv(int32_t fd, pk_slice_u8 buf) {
	net_RecvResult r;
	ssize_t ret = recv(fd, buf.data, buf.len, 0);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int64_t)ret; }
	return r;
}

void pk_net_close(int32_t fd) {
	close(fd);
}

/* UDP */

net_ListenResult pk_net_bind_udp(pk_str addr, uint16_t port) {
	net_ListenResult r;
	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) { r.tag = 1; r.val._1 = 0; return r; }
	pk_net_setsockopt_reuse(fd);
	struct sockaddr_in sa;
	pk_net_make_addr(addr, port, &sa);
	if (bind(fd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
		close(fd);
		r.tag = 1; r.val._1 = 0; return r;
	}
	r.tag = 0; r.val._0 = (int32_t)fd;
	return r;
}

net_SendResult pk_net_send_udp(int32_t fd, pk_slice_u8 data, pk_str addr, uint16_t port) {
	net_SendResult r;
	struct sockaddr_in sa;
	pk_net_make_addr(addr, port, &sa);
	ssize_t ret = sendto(fd, data.data, data.len, 0,
		(struct sockaddr*)&sa, sizeof(sa));
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int64_t)ret; }
	return r;
}

net_RecvResult pk_net_recv_udp(int32_t fd, pk_slice_u8 buf, pk_slice_u8 addr_buf, uint16_t* port_out) {
	net_RecvResult r;
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	socklen_t salen = sizeof(sa);

	ssize_t ret = recvfrom(fd, buf.data, buf.len, 0,
		(struct sockaddr*)&sa, &salen);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; return r; }

	if (addr_buf.data && addr_buf.len > 0) {
		uint32_t ip = ntohl(sa.sin_addr.s_addr);
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
	if (port_out) *port_out = ntohs(sa.sin_port);

	r.tag = 0; r.val._0 = (int64_t)ret;
	return r;
}

/* Multicast */

net_VoidResult pk_net_listen_mc(int32_t fd, pk_str group) {
	net_VoidResult r;
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = pk_parse_ipv4(group);
	mreq.imr_interface.s_addr = INADDR_ANY;
	int ret = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

net_VoidResult pk_net_leave_mc(int32_t fd, pk_str group) {
	net_VoidResult r;
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = pk_parse_ipv4(group);
	mreq.imr_interface.s_addr = INADDR_ANY;
	int ret = setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}
