#!/bin/sh
# Run all Pika benchmarks
set -e
cd "$(dirname "$0")/.."

BENCHMARKS="whetstone dhrystone mandelbrot livermore memspeed linpack"
arch="$(uname -m)"

case "$arch" in
	x86_64|amd64)
		bench_arch_flags="-march=native -mtune=native"
		gcc_math_libs="-lmvec -lm"
		clang_math_extra="-fveclib=libmvec -fuse-ld=lld"
		;;
	aarch64|arm64|armv7l|armv8l)
		bench_arch_flags="-mcpu=native"
		gcc_math_libs="-lm"
		clang_math_extra=""
		;;
	*)
		bench_arch_flags="-march=native -mtune=native"
		gcc_math_libs="-lm"
		clang_math_extra=""
		;;
esac

for b in $BENCHMARKS; do
	mkdir -p "build/benchmarks/$b"
done

platform=""
case "$(uname -m)" in
	aarch64|arm64) platform="--platform aarch64_raw" ;;
esac

echo "=== Pika whetstone ==="
bin/pikacc $platform benchmarks/bench.pka benchmarks/whetstone/whetstone.pka pikabench.c -o build/benchmarks/whetstone/whetstone
build/benchmarks/whetstone/whetstone

echo "=== Pika dhrystone ==="
CC=clang bin/pikacc $platform benchmarks/bench.pka benchmarks/dhrystone/dhrystone.pka pikabench.c -o build/benchmarks/dhrystone/dhrystone
build/benchmarks/dhrystone/dhrystone

echo "=== Pika mandelbrot ==="
CC=clang bin/pikacc $platform benchmarks/bench.pka benchmarks/mandelbrot/mandelbrot.pka pikabench.c -o build/benchmarks/mandelbrot/mandelbrot
build/benchmarks/mandelbrot/mandelbrot > /dev/null

echo "=== Pika livermore ==="
bin/pikacc $platform benchmarks/bench.pka benchmarks/livermore/livermore.pka pikabench.c -o build/benchmarks/livermore/livermore
build/benchmarks/livermore/livermore

echo "=== Pika memspeed ==="
bin/pikacc $platform benchmarks/bench.pka benchmarks/memspeed/memspeed.pka pikabench.c -o build/benchmarks/memspeed/memspeed
build/benchmarks/memspeed/memspeed

echo "=== Pika linpack ==="
bin/pikacc $platform benchmarks/bench.pka benchmarks/linpack/linpack.pka pikabench.c -o build/benchmarks/linpack/linpack
build/benchmarks/linpack/linpack

echo ""
echo "=== Math benchmark ==="
mkdir -p build/benchmarks
{ printf '%s\n' '-N math'; cat lib/math/math.pka; } | pika2c > build/benchmarks/math_benchmark.c
cat benchmarks/math_benchmark.c >> build/benchmarks/math_benchmark.c

echo "--- GCC ---"
gcc -D_GNU_SOURCE -std=c11 -Wall -Wextra -O3 -fno-strict-aliasing \
	-ffast-math -funroll-loops -ftree-vectorize $bench_arch_flags \
	-finline-functions -finline-limit=10000 -flto \
	build/benchmarks/math_benchmark.c \
	$gcc_math_libs \
	-o build/benchmarks/math_benchmark
./build/benchmarks/math_benchmark

echo "--- Clang ---"
clang -D_GNU_SOURCE -std=c11 -Wall -Wextra -fno-strict-aliasing \
	-O3 -ffast-math -ftree-vectorize $bench_arch_flags \
	-fvectorize -fslp-vectorize -fno-math-errno \
	-finline-functions -flto $clang_math_extra \
	build/benchmarks/math_benchmark.c $gcc_math_libs -o build/benchmarks/math_benchmark
./build/benchmarks/math_benchmark
