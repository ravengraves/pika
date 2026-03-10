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

# Integration check for the pikacc pipeline itself.
out1="build/test/stdlib/pikacc_pipeline_out"
out2="build/test/stdlib/pikacc_pipeline_out2"
rm -f "$out1" "$out2"
bin/pikacc test/basic.pka -O1 -o "$out1"
bin/pikacc test/runtime.pka -O1 -o "$out2"
[ -f "$out1" ]
[ -f "$out2" ]
[ -d build/tmp ]

echo "Stdlib tests passed."
