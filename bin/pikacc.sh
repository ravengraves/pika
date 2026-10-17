#!/usr/bin/env bash
# Pika compiler driver.
# Usage: pikacc <pka files...> [-c cfiles...] [ofiles...] [afiles...] -o <output>
#        [--platform x86_64_raw|aarch64_raw|libc|win64]

set -e

PLATFORM="x86_64_raw"
OUTPUT=""
OPTLEVEL="3"
MCPU=""
MARCH=""
MTUNE=""
PKAFILES=()
CFILES=()
OFILES=()

while [ "$#" -gt 0 ]; do
  case "$1" in
    -o) shift; OUTPUT="$1" ;;
    --platform) shift; PLATFORM="$1" ;;
    --mcpu) shift; MCPU="$1" ;;
    --march) shift; MARCH="$1" ;;
    --mtune) shift; MTUNE="$1" ;;
    -O[s0123]) OPTLEVEL="${1#-O}" ;;
    *.c) CFILES+=( "$1" ) ;;
    *.o|*.a) OFILES+=( "$1" ) ;;
    *) PKAFILES+=( "$1" ) ;;
  esac
  shift
done

if [ -z "$OUTPUT" ] || [ ${#PKAFILES[@]} -eq 0 ]; then
  echo "Usage: $0 <pka files...> [--platform x86_64_raw|aarch64_raw|libc|win64] -o <output>" >&2
  exit 1
fi

# --- Compiler selection ---
case "$PLATFORM" in
  win64) CC="${CC:-x86_64-w64-mingw32-gcc}" ;;
  *)     CC="${CC:-gcc}" ;;
esac

# --- Flags ---
CFLAGS_COMMON="-std=c11 -Wall -Wextra -Wconversion -Wfloat-conversion \
-Wno-overlength-strings -Wno-unused-function -Wno-unused-variable \
-ffunction-sections -fdata-sections \
-O${OPTLEVEL} -ffast-math -fno-finite-math-only -funroll-loops -ftree-vectorize \
-finline-functions -fno-strict-aliasing"

# Avoid passing Clang-only warning flags to GCC.
CC_VERSION=$($CC --version 2>/dev/null | head -n1 || true)
# If $CC is clang (or anything that contains the word “clang”),
# add the warning‑suppressions that only clang understands.
if [[ "$(basename "$CC")" == clang ]]; then
  CFLAGS_COMMON+=" -Wno-gnu-auto-type -Wno-gnu-statement-expression -Wno-parentheses-equality"
fi

case "$PLATFORM" in
  x86_64_raw)
    CFLAGS="$CFLAGS_COMMON -march=native -mtune=native -mshstk \
-fno-builtin -ffreestanding -nostdlib -fno-asynchronous-unwind-tables -fno-unwind-tables \
-fno-stack-protector -fno-ident \
-Wl,--gc-sections,--build-id=none,--strip-all -flto -static -s" ;;
  aarch64_raw)
    CFLAGS="$CFLAGS_COMMON \
-mcpu=native \
-mno-outline-atomics \
-fno-builtin -ffreestanding -nostdlib -fno-asynchronous-unwind-tables -fno-unwind-tables \
-fno-stack-protector -fno-ident \
-Wl,--gc-sections,--build-id=none,--strip-all -flto -static -s" ;;
  libc)
    CFLAGS="$CFLAGS_COMMON -march=native -mtune=native \
-D_GNU_SOURCE -Wno-unused-result -Wl,--gc-sections -flto -s"
    LIBS="${LIBS:-} -lrt" ;;
  win64)
    CFLAGS="$CFLAGS_COMMON \
-Wno-unused-result -mconsole -Wl,--gc-sections -flto -s"
    LIBS="${LIBS:-} -lws2_32 -lbcrypt" ;;
  *)
    echo "Unknown platform: $PLATFORM" >&2; exit 1 ;;
esac

if [ -n "${CFLAGS_EXTRA:-}" ]; then
  CFLAGS="$CFLAGS $CFLAGS_EXTRA"
fi
if [ -n "$MCPU" ]; then
  CFLAGS="$CFLAGS -mcpu=$MCPU"
fi
if [ -n "$MARCH" ]; then
  CFLAGS="$CFLAGS -march=$MARCH"
fi
if [ -n "$MTUNE" ]; then
  CFLAGS="$CFLAGS -mtune=$MTUNE"
fi

# Prefer installed resources, falling back to the checkout.
DATA_DIR=.
if [ -d /usr/local/share/pika/lib ] && [ -d /usr/local/share/pika/runtime ]; then
  DATA_DIR=/usr/local/share/pika
fi

# --- Runtime selection ---
case "$PLATFORM" in
  x86_64_raw|aarch64_raw) RT_FILE="$DATA_DIR/runtime/rt.c" ;;
  libc)                    RT_FILE="$DATA_DIR/runtime/rt_libc.c" ;;
  win64)                   RT_FILE="$DATA_DIR/runtime/rt_win64.c" ;;
esac

# --- Temp files (cleaned on exit) ---
TMPFILE=$(mktemp /tmp/pikacc-XXXXXX.c)
LIBPK_C=""
TMPPKA=$(mktemp /tmp/pikacc-pka-XXXXXX.pka)
cleanup() { rm -f "$TMPFILE" "$LIBPK_C" "$TMPPKA"; }
trap cleanup EXIT

RT_FWD='#include <stddef.h>
extern int rt_memcmp(const void*, const void*, size_t);
'

PIKA2C="pika2c"

# Map platform to target suffix
case "$PLATFORM" in
  x86_64_raw)  LIB_TARGET=x86_64 ;;
  aarch64_raw) LIB_TARGET=aarch64 ;;
  libc)        LIB_TARGET=libc ;;
  win64)       LIB_TARGET=win ;;
esac

# Discover lib modules and platform C files
LIBPK_MODULES=()
while IFS= read -r f; do
  LIBPK_MODULES+=("$f")
done < <(find "$DATA_DIR/lib" -name '*.pka' -type f | sort)

PLATFORM_H="$DATA_DIR/lib/platform+${LIB_TARGET}.h"
TARGET_PATTERNS=( -name "*+${LIB_TARGET}.c" )
case "$PLATFORM" in
  x86_64_raw|aarch64_raw)
    TARGET_PATTERNS+=( -o -name "*+raw.c" ) ;;
esac

COMMON_C=()
while IFS= read -r f; do
  COMMON_C+=("$f")
done < <(find "$DATA_DIR/lib" -type f -name '*.c' ! -name '*+*.c' | sort)

TARGET_C=()
while IFS= read -r f; do
  TARGET_C+=("$f")
done < <(find "$DATA_DIR/lib" -type f \( "${TARGET_PATTERNS[@]}" \) | sort)

# Concatenate platform C backing into temp file
LIBPK_C=$(mktemp /tmp/libpka-XXXXXX.c)
cat "$PLATFORM_H" "${COMMON_C[@]}" "${TARGET_C[@]}" > "$LIBPK_C"

# --- Build single translation unit ---
{
  printf '%s' "$RT_FWD"
  if [ ${#CFILES[@]} -gt 0 ]; then cat "${CFILES[@]}"; fi
  # Compile each lib module with -N <modname>
  for pka in "${LIBPK_MODULES[@]}"; do
    modname=$(basename "$(dirname "$pka")")
    { printf '%s\n' "-N $modname"; cat "$pka"; } > "$TMPPKA"
    "$PIKA2C" < "$TMPPKA"
  done
  # Compile user .pka files with lib headers for types/defaults/symbols
  {
    for pka in "${LIBPK_MODULES[@]}"; do
      modname=$(basename "$(dirname "$pka")")
      { printf '%s\n' "-H"; printf '%s\n' "-N $modname"; cat "$pka"; } > "$TMPPKA"
      "$PIKA2C" < "$TMPPKA"
    done
    cat "${PKAFILES[@]}"
  } | "$PIKA2C"
  cat "$LIBPK_C"
  cat "$RT_FILE"
} > "$TMPFILE"

# --- Compile ---
start_ms=$(date +%s%3N)
$CC $CFLAGS -o "$OUTPUT" "$TMPFILE" "${OFILES[@]}" ${LIBS:-}
end_ms=$(date +%s%3N)
duration=$((end_ms - start_ms))

echo "Built $OUTPUT"
echo "${duration} ms"
