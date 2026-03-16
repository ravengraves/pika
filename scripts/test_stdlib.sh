#!/bin/sh
# Run stdlib tests
set -e
cd "$(dirname "$0")/.."

platform=""
case "$(uname -m)" in
	aarch64|arm64) platform="--platform aarch64_raw" ;;
esac

mkdir -p build/test/stdlib
for i in test/stdlib/*.pka; do
	base="${i%.*}"
	out="build/$base"
	name="${base##*/}"
	echo "building $out ..."
	bin/pikacc $platform "$i" -O1 -o "$out"
	if [ "$name" = "args" ]; then
		# This test validates flag and positional parsing, so it needs argv.
		"$out" --flag value
	elif [ "$name" = "env" ]; then
		PIKA_ENV_TEST=rose PIKA_ENV_EMPTY= "$out"
	else
		"$out"
	fi
done

# Integration check for the pikacc pipeline itself.
out1="build/test/stdlib/pikacc_pipeline_out"
out2="build/test/stdlib/pikacc_pipeline_out2"
rm -f "$out1" "$out2"
bin/pikacc $platform test/basic.pka -O1 -o "$out1"
bin/pikacc $platform test/runtime.pka -O1 -o "$out2"
[ -f "$out1" ]
[ -f "$out2" ]
[ -d build/tmp ]

echo "Stdlib tests passed."
