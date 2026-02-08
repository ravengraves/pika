/* pk_crypt.c — Cryptographic primitives for Windows (MinGW) */

crypt_RandomResult pk_crypt_random(pk_slice_u8 buf) {
	crypt_RandomResult r;
	/* BCryptGenRandom with BCRYPT_USE_SYSTEM_PREFERRED_RNG (=2) */
	NTSTATUS status = BCryptGenRandom(NULL, buf.data, (ULONG)buf.len,
		BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	if (status == 0) { /* STATUS_SUCCESS */
		r.tag = 0; r.val._0 = 0;
	} else {
		r.tag = 1; r.val._1 = 0;
	}
	return r;
}
