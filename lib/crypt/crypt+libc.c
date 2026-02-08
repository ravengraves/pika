/* pk_crypt.c — Cryptographic primitives using libc (POSIX) */

crypt_RandomResult pk_crypt_random(pk_slice_u8 buf) {
	crypt_RandomResult r;
#ifdef __APPLE__
	arc4random_buf(buf.data, buf.len);
	r.tag = 0; r.val._0 = 0;
#else
	ssize_t ret = getrandom(buf.data, buf.len, 0);
	if (ret < 0 || (size_t)ret != buf.len) {
		r.tag = 1; r.val._1 = 0;
	} else {
		r.tag = 0; r.val._0 = 0;
	}
#endif
	return r;
}
