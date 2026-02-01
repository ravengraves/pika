
check:
	mkdir -p build/test
	for i in test/*.pka; do \
		echo building $$i ... && \
		bin/pikacc $$i -o build/$$i.bin && \
		build/$$i.bin; \
	done

	mkdir -p build/test/stdlib
	for i in ring_writer ring_server ring_client ring_supervisor; \
		do \
		echo building $$i ... && \
		bin/pikacc \
			test/stdlib/ring_common.pka \
			test/stdlib/$$i.pka \
			-o build/test/stdlib/$$i.bin; \
	done

.PHONY: check