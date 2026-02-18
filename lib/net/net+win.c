/* pk_net.c — Networking for Windows (MinGW) via Winsock2 */

static void pk_net_make_addr(pk_str addr, uint16_t port, struct sockaddr_in* sa) {
	for (size_t i = 0; i < sizeof(*sa); i++) ((uint8_t*)sa)[i] = 0;
	sa->sin_family = AF_INET;
	sa->sin_port = pk_htons(port);
	sa->sin_addr.s_addr = pk_parse_ipv4(addr);
}

static int pk_net_setsockopt_reuse(SOCKET s) {
	int optval = 1;
	return setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));
}

/* TCP */

net_ListenResult pk_net_listen(pk_str addr, uint16_t port, int32_t backlog) {
	net_ListenResult r;
	pk__ensure_winsock();

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) { r.tag = 1; r.val._1 = 0; return r; }

	pk_net_setsockopt_reuse(s);

	struct sockaddr_in sa;
	pk_net_make_addr(addr, port, &sa);

	if (bind(s, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR) {
		closesocket(s);
		r.tag = 1; r.val._1 = 0; return r;
	}
	if (listen(s, backlog) == SOCKET_ERROR) {
		closesocket(s);
		r.tag = 1; r.val._1 = 0; return r;
	}

	r.tag = 0; r.val._0 = (int32_t)s;
	return r;
}

net_AcceptResult pk_net_accept(int32_t fd) {
	net_AcceptResult r;
	pk__ensure_winsock();

	SOCKET s = accept((SOCKET)(intptr_t)fd, NULL, NULL);
	if (s == INVALID_SOCKET) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int32_t)s; }
	return r;
}

net_ConnectResult pk_net_connect(pk_str addr, uint16_t port) {
	net_ConnectResult r;
	pk__ensure_winsock();

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) { r.tag = 1; r.val._1 = 0; return r; }

	struct sockaddr_in sa;
	pk_net_make_addr(addr, port, &sa);

	if (connect(s, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR) {
		closesocket(s);
		r.tag = 1; r.val._1 = 0; return r;
	}

	r.tag = 0; r.val._0 = (int32_t)s;
	return r;
}

net_SendResult pk_net_send(int32_t fd, pk_slice_u8 data) {
	net_SendResult r;
	pk__ensure_winsock();

	int ret = send((SOCKET)(intptr_t)fd, (const char*)data.data, (int)data.len, 0);
	if (ret == SOCKET_ERROR) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int64_t)ret; }
	return r;
}

net_RecvResult pk_net_recv(int32_t fd, pk_slice_u8 buf) {
	net_RecvResult r;
	pk__ensure_winsock();

	int ret = recv((SOCKET)(intptr_t)fd, (char*)buf.data, (int)buf.len, 0);
	if (ret == SOCKET_ERROR) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int64_t)ret; }
	return r;
}

void pk_net_close(int32_t fd) {
	pk__ensure_winsock();
	closesocket((SOCKET)(intptr_t)fd);
}

/* UDP */

net_ListenResult pk_net_bind_udp(pk_str addr, uint16_t port) {
	net_ListenResult r;
	pk__ensure_winsock();

	SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET) { r.tag = 1; r.val._1 = 0; return r; }

	pk_net_setsockopt_reuse(s);

	struct sockaddr_in sa;
	pk_net_make_addr(addr, port, &sa);

	if (bind(s, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR) {
		closesocket(s);
		r.tag = 1; r.val._1 = 0; return r;
	}

	r.tag = 0; r.val._0 = (int32_t)s;
	return r;
}

net_SendResult pk_net_send_udp(int32_t fd, pk_slice_u8 data, pk_str addr, uint16_t port) {
	net_SendResult r;
	pk__ensure_winsock();

	struct sockaddr_in sa;
	pk_net_make_addr(addr, port, &sa);

	int ret = sendto((SOCKET)(intptr_t)fd, (const char*)data.data, (int)data.len, 0,
		(struct sockaddr*)&sa, sizeof(sa));
	if (ret == SOCKET_ERROR) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = (int64_t)ret; }
	return r;
}

net_RecvResult pk_net_recv_udp(int32_t fd, pk_slice_u8 buf, pk_slice_u8 addr_buf, uint16_t* port_out) {
	net_RecvResult r;
	pk__ensure_winsock();

	struct sockaddr_in sa;
	for (size_t i = 0; i < sizeof(sa); i++) ((uint8_t*)&sa)[i] = 0;
	int salen = (int)sizeof(sa);

	int ret = recvfrom((SOCKET)(intptr_t)fd, (char*)buf.data, (int)buf.len, 0,
		(struct sockaddr*)&sa, &salen);
	if (ret == SOCKET_ERROR) { r.tag = 1; r.val._1 = 0; return r; }

	pk_net_format_ipv4(addr_buf, sa.sin_addr.s_addr);
	if (port_out) *port_out = pk_htons(sa.sin_port);

	r.tag = 0; r.val._0 = (int64_t)ret;
	return r;
}

/* Multicast */

net_VoidResult pk_net_listen_mc(int32_t fd, pk_str group) {
	net_VoidResult r;
	pk__ensure_winsock();

	struct ip_mreq mreq;
	for (size_t i = 0; i < sizeof(mreq); i++) ((uint8_t*)&mreq)[i] = 0;
	mreq.imr_multiaddr.s_addr = pk_parse_ipv4(group);
	mreq.imr_interface.s_addr = 0;

	int ret = setsockopt((SOCKET)(intptr_t)fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		(const char*)&mreq, sizeof(mreq));
	if (ret == SOCKET_ERROR) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

net_VoidResult pk_net_leave_mc(int32_t fd, pk_str group) {
	net_VoidResult r;
	pk__ensure_winsock();

	struct ip_mreq mreq;
	for (size_t i = 0; i < sizeof(mreq); i++) ((uint8_t*)&mreq)[i] = 0;
	mreq.imr_multiaddr.s_addr = pk_parse_ipv4(group);
	mreq.imr_interface.s_addr = 0;

	int ret = setsockopt((SOCKET)(intptr_t)fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		(const char*)&mreq, sizeof(mreq));
	if (ret == SOCKET_ERROR) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}
