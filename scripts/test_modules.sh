#!/bin/sh
set -e
cd "$(dirname "$0")/.."

mkdir -p build/test/modules
for source in test/modules/*.pka; do
    name=${source##*/}
    bin/pikacc "$source" --platform libc -M test/modules/lib -o "build/test/modules/${name%.pka}"
done
echo "All four module import programs built."
