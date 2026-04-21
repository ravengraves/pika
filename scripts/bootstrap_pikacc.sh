#!/bin/sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$ROOT_DIR/build"
"$ROOT_DIR/bin/pikacc.sh" "$ROOT_DIR/bin/pikacc.pka" --platform libc -O1 -o "$ROOT_DIR/build/pikacc"
cp "$ROOT_DIR/build/pikacc" bin/pikacc
