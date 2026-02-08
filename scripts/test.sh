#!/bin/sh
# Run language tests
set -e
cd "$(dirname "$0")/.."

mkdir -p build/test
for i in test/*.pka; do
	base="${i%.*}"
	echo "building build/$base ..."
	bin/pikacc "$i" -o "build/$base"
	"build/$base"
done
echo "All language tests passed."
