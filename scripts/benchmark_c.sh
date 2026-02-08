#!/bin/sh
# Run C reference benchmarks
set -e
cd "$(dirname "$0")/.."

BENCH_CC="${BENCH_CC:-gcc}"
BENCH_CFLAGS="-O3 -ffast-math -funroll-loops -ftree-vectorize -march=native -mtune=native -finline-functions -fno-strict-aliasing -flto"

for b in whetstone dhrystone livermore memspeed linpack mandelbrot; do
	mkdir -p "build/benchmarks/$b"
done

for b in whetstone dhrystone livermore memspeed linpack; do
	echo "=== C $b ==="
	$BENCH_CC $BENCH_CFLAGS -o "build/benchmarks/$b/${b}_c" "benchmarks/c_reference/${b}_ref.c" -lm
	"build/benchmarks/$b/${b}_c"
done

echo "=== C mandelbrot ==="
$BENCH_CC $BENCH_CFLAGS -fno-finite-math-only -fopenmp -o build/benchmarks/mandelbrot/mandelbrot_c benchmarks/c_reference/mandelbrot_ref.c
time OMP_NUM_THREADS=1 build/benchmarks/mandelbrot/mandelbrot_c 16000 > /dev/null
