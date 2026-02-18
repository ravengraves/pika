/* pk_crypt.c — Cryptographic primitives for Linux raw syscalls */

crypt_RandomResult pk_crypt_random(pk_slice_u8 buf) {
	crypt_RandomResult r;
	int64_t ret = pk_syscall3(__PK_SYS_getrandom, (int64_t)buf.data, (int64_t)buf.len, 0);
	if (ret < 0 || (size_t)ret != buf.len) {
		r.tag = 1; r.val._1 = 0;
	} else {
		r.tag = 0; r.val._0 = 0;
	}
	return r;
}
