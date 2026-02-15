#!/bin/sh
# Run stdlib tests
set -e
cd "$(dirname "$0")/.."

mkdir -p build/test/stdlib
for i in test/stdlib/*.pka; do
	base="${i%.*}"
	out="build/$base"
	echo "building $out ..."
	bin/pikacc "$i" -O1 -o "$out"
	"$out"
done
echo "Stdlib tests passed."
