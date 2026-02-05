#define _POSIX_C_SOURCE 199309L
check: check-lang check-stdlib check-math

check-lang:
	mkdir -p build/test
	for i in test/*.pka; do \
		base="$${i%.*}" && \
		echo building build/$$base ... && \
		bin/pikacc $$i -o build/$$base && \
		build/$$base; \
	done

check-stdlib:
	mkdir -p build/test/stdlib
	set -e;
	for i in ring_writer ring_server ring_client ring_supervisor; \
		do \
			echo building $$i ... && \
			bin/pikacc \
				test/stdlib/ring_common.pka \
				test/stdlib/$$i.pka \
				-o build/test/stdlib/$$i; \
	done

	cd build/test/stdlib && \
		timeout 1s ./ring_supervisor > out.txt || true && \
		if [ "$$(wc -l out.txt | awk '{print $$1}')" -lt 10 ]; \
		then echo "Not enough lines" && false; \
		fi

check-math:
	cat stdlib/math.pka | ./bin/pika2c > build/test/math.c
	cat test/math_test.c >> build/test/math.c
	gcc -std=c11 -Wall -Wextra -O3 -fno-strict-aliasing \
		-O3 -ffast-math -funroll-loops -ftree-vectorize -march=native -mtune=native \
		-finline-functions  \
		 build/test/math.c -lm -o build/test/math_test
	./build/test/math_test

	cat stdlib/math.pka | ./bin/pika2c > build/test/math.c > build/test/math_benchmark.c
	cat test/math_benchmark.c >> build/test/math_benchmark.c
	echo ===================== GCC =====================
	gcc -D_GNU_SOURCE -std=c11 -Wall -Wextra -O3 -fno-strict-aliasing \
		-O3 -ffast-math -funroll-loops -ftree-vectorize -march=native -mtune=native \
		-finline-functions -finline-limit=10000 -flto \
		build/test/math_benchmark.c  \
		-lmvec -lm \
		-o build/test/math_benchmark
	./build/test/math_benchmark

	echo ===================== Clang =====================
	clang -D_GNU_SOURCE -std=c11 -Wall -Wextra -fno-strict-aliasing \
		-O3 -ffast-math -ftree-vectorize -march=native -mtune=native \
		-fvectorize -fslp-vectorize -fno-math-errno -fveclib=libmvec \
		-finline-functions -flto -fuse-ld=lld \
		build/test/math_benchmark.c -lmvec -lm -o build/test/math_benchmark
	./build/test/math_benchmark


clean:
	# Remove generated objects and binaries
	rm -rf build/

.PHONY: check check-lang check-stdlib check-math clean
