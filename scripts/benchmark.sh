#!/bin/sh
# Run all Pika benchmarks
set -e
cd "$(dirname "$0")/.."

BENCHMARKS="whetstone dhrystone mandelbrot livermore memspeed linpack"

for b in $BENCHMARKS; do
	mkdir -p "build/benchmarks/$b"
done

echo "=== Pika whetstone ==="
bin/pikacc benchmarks/bench.pka benchmarks/whetstone/whetstone.pka pikabench.c -o build/benchmarks/whetstone/whetstone
build/benchmarks/whetstone/whetstone

echo "=== Pika dhrystone ==="
CC=clang bin/pikacc benchmarks/bench.pka benchmarks/dhrystone/dhrystone.pka pikabench.c -o build/benchmarks/dhrystone/dhrystone
build/benchmarks/dhrystone/dhrystone

echo "=== Pika mandelbrot ==="
CC=clang bin/pikacc benchmarks/bench.pka benchmarks/mandelbrot/mandelbrot.pka pikabench.c -o build/benchmarks/mandelbrot/mandelbrot
time build/benchmarks/mandelbrot/mandelbrot > /dev/null

echo "=== Pika livermore ==="
bin/pikacc benchmarks/bench.pka benchmarks/livermore/livermore.pka pikabench.c -o build/benchmarks/livermore/livermore
build/benchmarks/livermore/livermore

echo "=== Pika memspeed ==="
bin/pikacc benchmarks/bench.pka benchmarks/memspeed/memspeed.pka pikabench.c -o build/benchmarks/memspeed/memspeed
build/benchmarks/memspeed/memspeed

echo "=== Pika linpack ==="
bin/pikacc benchmarks/bench.pka benchmarks/linpack/linpack.pka pikabench.c -o build/benchmarks/linpack/linpack
build/benchmarks/linpack/linpack

echo ""
echo "=== Math benchmark ==="
mkdir -p build/benchmarks
cat stdlib/math.pka | ./bin/pika2c > build/benchmarks/math_benchmark.c
cat benchmarks/math_benchmark.c >> build/benchmarks/math_benchmark.c

echo "--- GCC ---"
gcc -D_GNU_SOURCE -std=c11 -Wall -Wextra -O3 -fno-strict-aliasing \
	-ffast-math -funroll-loops -ftree-vectorize -march=native -mtune=native \
	-finline-functions -finline-limit=10000 -flto \
	build/benchmarks/math_benchmark.c \
	-lmvec -lm \
	-o build/benchmarks/math_benchmark
./build/benchmarks/math_benchmark

echo "--- Clang ---"
clang -D_GNU_SOURCE -std=c11 -Wall -Wextra -fno-strict-aliasing \
	-O3 -ffast-math -ftree-vectorize -march=native -mtune=native \
	-fvectorize -fslp-vectorize -fno-math-errno -fveclib=libmvec \
	-finline-functions -flto -fuse-ld=lld \
	build/benchmarks/math_benchmark.c -lmvec -lm -o build/benchmarks/math_benchmark
./build/benchmarks/math_benchmark
