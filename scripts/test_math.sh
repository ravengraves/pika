#!/bin/sh
# Run math accuracy tests
set -e
cd "$(dirname "$0")/.."

mkdir -p build/test
cat stdlib/math.pka | ./bin/pika2c > build/test/math.c
cat test/math_test.c >> build/test/math.c
gcc -std=c11 -Wall -Wextra -O3 -fno-strict-aliasing \
	-O3 -ffast-math -funroll-loops -ftree-vectorize -march=native -mtune=native \
	-finline-functions \
	build/test/math.c -lm -o build/test/math_test
./build/test/math_test
