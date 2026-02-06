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

BENCH_CC ?= gcc
BENCH_CFLAGS=-O3 -ffast-math -funroll-loops -ftree-vectorize -march=native -mtune=native -finline-functions -fno-strict-aliasing -flto

benchmark: benchmark-whetstone benchmark-dhrystone benchmark-mandelbrot benchmark-livermore benchmark-memspeed benchmark-linpack bench-math

BENCH_RUSTFLAGS=-C opt-level=3 -C target-cpu=native

benchmark-rust:
	@mkdir -p build/benchmarks/{whetstone,dhrystone,livermore,memspeed,linpack,mandelbrot}
	@for b in whetstone dhrystone livermore memspeed linpack; do \
		echo "=== Rust $$b ===" && \
		rustc $(BENCH_RUSTFLAGS) -o build/benchmarks/$$b/$${b}_rs benchmarks/rust_reference/$${b}_ref.rs && \
		build/benchmarks/$$b/$${b}_rs; \
	done
	@echo "=== Rust mandelbrot ==="
	rustc $(BENCH_RUSTFLAGS) -o build/benchmarks/mandelbrot/mandelbrot_rs benchmarks/rust_reference/mandelbrot_ref.rs
	bash -c 'time build/benchmarks/mandelbrot/mandelbrot_rs > /dev/null'

benchmark-c:
	@mkdir -p build/benchmarks/{whetstone,dhrystone,livermore,memspeed,linpack,mandelbrot}
	@for b in whetstone dhrystone livermore memspeed linpack; do \
		echo "=== C $$b ===" && \
		$(BENCH_CC) $(BENCH_CFLAGS) -o build/benchmarks/$$b/$${b}_c benchmarks/c_reference/$${b}_ref.c -lm && \
		build/benchmarks/$$b/$${b}_c; \
	done
	@echo "=== C mandelbrot ==="
	$(BENCH_CC) $(BENCH_CFLAGS) -fno-finite-math-only -fopenmp -o build/benchmarks/mandelbrot/mandelbrot_c benchmarks/c_reference/mandelbrot_ref.c
	bash -c 'time OMP_NUM_THREADS=1 build/benchmarks/mandelbrot/mandelbrot_c 16000 > /dev/null'

benchmark-whetstone:
	mkdir -p build/benchmarks/whetstone
	bin/pikacc benchmarks/bench.pka benchmarks/whetstone/whetstone.pka pikabench.c -o build/benchmarks/whetstone/whetstone
	build/benchmarks/whetstone/whetstone

benchmark-dhrystone:
	mkdir -p build/benchmarks/dhrystone
	CC=clang bin/pikacc benchmarks/bench.pka benchmarks/dhrystone/dhrystone.pka pikabench.c -o build/benchmarks/dhrystone/dhrystone
	build/benchmarks/dhrystone/dhrystone

benchmark-mandelbrot:
	mkdir -p build/benchmarks/mandelbrot
	CC=clang bin/pikacc benchmarks/bench.pka benchmarks/mandelbrot/mandelbrot.pka pikabench.c -o build/benchmarks/mandelbrot/mandelbrot
	bash -c 'time build/benchmarks/mandelbrot/mandelbrot > /dev/null'

benchmark-livermore:
	mkdir -p build/benchmarks/livermore
	bin/pikacc benchmarks/bench.pka benchmarks/livermore/livermore.pka pikabench.c -o build/benchmarks/livermore/livermore
	build/benchmarks/livermore/livermore

benchmark-memspeed:
	mkdir -p build/benchmarks/memspeed
	bin/pikacc benchmarks/bench.pka benchmarks/memspeed/memspeed.pka pikabench.c -o build/benchmarks/memspeed/memspeed
	build/benchmarks/memspeed/memspeed

benchmark-linpack:
	mkdir -p build/benchmarks/linpack
	bin/pikacc benchmarks/bench.pka benchmarks/linpack/linpack.pka pikabench.c -o build/benchmarks/linpack/linpack
	build/benchmarks/linpack/linpack

bench: bench-math

bench-math:
	mkdir -p build/benchmarks
	cat stdlib/math.pka | ./bin/pika2c > build/benchmarks/math_benchmark.c
	cat benchmarks/math_benchmark.c >> build/benchmarks/math_benchmark.c

	echo ===================== GCC =====================
	gcc -D_GNU_SOURCE -std=c11 -Wall -Wextra -O3 -fno-strict-aliasing \
		-ffast-math -funroll-loops -ftree-vectorize -march=native -mtune=native \
		-finline-functions -finline-limit=10000 -flto \
		build/benchmarks/math_benchmark.c \
		-lmvec -lm \
		-o build/benchmarks/math_benchmark
	./build/benchmarks/math_benchmark

	echo ===================== Clang =====================
	clang -D_GNU_SOURCE -std=c11 -Wall -Wextra -fno-strict-aliasing \
		-O3 -ffast-math -ftree-vectorize -march=native -mtune=native \
		-fvectorize -fslp-vectorize -fno-math-errno -fveclib=libmvec \
		-finline-functions -flto -fuse-ld=lld \
		build/benchmarks/math_benchmark.c -lmvec -lm -o build/benchmarks/math_benchmark
	./build/benchmarks/math_benchmark


clean:
	# Remove generated objects and binaries
	rm -rf build/

.PHONY: check check-lang check-stdlib check-math bench bench-math \
	benchmark benchmark-whetstone benchmark-dhrystone benchmark-mandelbrot \
	benchmark-livermore benchmark-memspeed benchmark-linpack \
	benchmark-c benchmark-rust clean
