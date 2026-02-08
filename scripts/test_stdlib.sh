#!/bin/sh
# Run stdlib tests (ring buffer supervisor test)
set -e
cd "$(dirname "$0")/.."

mkdir -p build/test/stdlib
for i in ring_writer ring_server ring_client ring_supervisor; do
	echo "building $i ..."
	bin/pikacc \
		test/stdlib/ring_common.pka \
		test/stdlib/${i}.pka \
		-o build/test/stdlib/${i}
done

cd build/test/stdlib
timeout 1s ./ring_supervisor > out.txt || true
lines=$(wc -l < out.txt)
if [ "$lines" -lt 10 ]; then
	echo "Not enough lines ($lines)" && exit 1
fi
echo "Stdlib tests passed."
