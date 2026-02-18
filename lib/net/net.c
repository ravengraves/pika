/* pk_net.c — backend-agnostic networking helpers */

void pk_net_format_ipv4(pk_slice_u8 addr_buf, uint32_t net_ip) {
	if (!addr_buf.data || addr_buf.len == 0) return;

	uint32_t ip = pk_htonl(net_ip);
	uint8_t a = (uint8_t)(ip >> 24), b = (uint8_t)(ip >> 16);
	uint8_t c = (uint8_t)(ip >> 8), d = (uint8_t)(ip);
	size_t pos = 0;
	uint8_t parts[4] = {a, b, c, d};

	for (int p = 0; p < 4 && pos < addr_buf.len; p++) {
		if (p > 0 && pos < addr_buf.len) addr_buf.data[pos++] = '.';
		uint8_t v = parts[p];
		if (v >= 100 && pos < addr_buf.len) {
			addr_buf.data[pos++] = (uint8_t)('0' + v / 100);
			v %= 100;
		}
		if (parts[p] >= 10 && pos < addr_buf.len) {
			addr_buf.data[pos++] = (uint8_t)('0' + v / 10);
			v %= 10;
		}
		if (pos < addr_buf.len) addr_buf.data[pos++] = (uint8_t)('0' + v);
	}

	if (pos < addr_buf.len) addr_buf.data[pos] = 0;
}
