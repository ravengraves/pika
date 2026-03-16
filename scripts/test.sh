#!/bin/sh
# Run language tests
set -e
cd "$(dirname "$0")/.."

platform=""
case "$(uname -m)" in
	aarch64|arm64) platform="--platform aarch64_raw" ;;
esac

mkdir -p build/test
for i in test/*.pka; do
	base="${i%.*}"
	echo "building build/$base ..."
	bin/pikacc $platform "$i" -o "build/$base"
	"build/$base"
done
echo "All language tests passed."
