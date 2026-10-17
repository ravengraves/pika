#!/bin/sh
# Check C argument forwarding with quoted paths and ordered static libraries.
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/c libs"

cat > "$tmp/main.pka" <<'EOF'
@symbol("driver_value") fn value() i32;
export fn main() void = { assert(value() == 42); };
EOF
cat > "$tmp/c libs/config.h" <<'EOF'
#define EXPECTED 42
EOF
cat > "$tmp/bridge.c" <<'EOF'
#include <config.h>
#include <string.h>
extern int first(void);
int driver_value(void) {
    return strcmp(MESSAGE, "hello world") == 0 && first() == EXPECTED ? 42 : 0;
}
EOF
cat > "$tmp/first.c" <<'EOF'
extern int second(void);
int first(void) { return second(); }
EOF
cat > "$tmp/second.c" <<'EOF'
int second(void) { return 42; }
EOF
gcc -c "$tmp/first.c" -o "$tmp/first.o"
gcc -c "$tmp/second.c" -o "$tmp/second.o"
ar rcs "$tmp/c libs/libfirst.a" "$tmp/first.o"
ar rcs "$tmp/c libs/libsecond.a" "$tmp/second.o"

for driver in bin/pikacc bin/pikacc.sh; do
    # Raw builds need compiler runtime helpers even without optimization.
    set --
    case "$(uname -m)" in
        aarch64|arm64) set -- --platform aarch64_raw ;;
    esac
    "$driver" test/modules/math.pka -o "$tmp/raw" "$@"
    "$tmp/raw"
    for mode in default release; do
        set --
        if [ "$mode" = release ]; then set -- -R; fi
        "$driver" "$tmp/main.pka" --platform libc -o "$tmp/app" "$@" -- \
            -O1 -I "$tmp/c libs" '-DMESSAGE="hello world"' "$tmp/bridge.c" \
            -L "$tmp/c libs" -lfirst -lsecond
        "$tmp/app"
    done
done
echo "Driver argument forwarding passed with and without -R."
