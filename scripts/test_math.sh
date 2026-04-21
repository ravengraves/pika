#!/bin/sh
# Run math accuracy tests
set -e
cd "$(dirname "$0")/.."

mkdir -p build/test
{ printf '%s\n' '-N math'; cat lib/math/math.pka; } | pika2c > build/test/math.c
cat test/math_test.c >> build/test/math.c
gcc -std=c11 -Wall -Wextra -O1 -fno-strict-aliasing \
	-ffast-math -funroll-loops -march=native -mtune=native \
	-finline-functions \
	build/test/math.c -lm -o build/test/math_test
./build/test/math_test
