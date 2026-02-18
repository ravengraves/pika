#!/bin/sh
# Run Rust reference benchmarks
set -e
cd "$(dirname "$0")/.."

BENCH_RUSTFLAGS="-C opt-level=3 -C target-cpu=native"

for b in whetstone dhrystone livermore memspeed linpack mandelbrot; do
	mkdir -p "build/benchmarks/$b"
done

for b in whetstone dhrystone livermore memspeed linpack; do
	echo "=== Rust $b ==="
	rustc $BENCH_RUSTFLAGS -o "build/benchmarks/$b/${b}_rs" "benchmarks/rust_reference/${b}_ref.rs"
	"build/benchmarks/$b/${b}_rs"
done

echo "=== Rust mandelbrot ==="
rustc $BENCH_RUSTFLAGS -o build/benchmarks/mandelbrot/mandelbrot_rs benchmarks/rust_reference/mandelbrot_ref.rs
build/benchmarks/mandelbrot/mandelbrot_rs > /dev/null
