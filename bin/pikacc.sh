#!/usr/bin/env bash
# Pika compiler driver.
# Usage: pikacc <pka files...> [cfiles...] [ofiles...] [afiles...] -o <output>
#        [-R] [--platform x86_64_raw|aarch64_raw|libc|win64] [-- <cc args...>]

set -e

PLATFORM="x86_64_raw"
OUTPUT=""
RELEASE=0
CC_ARGS=()
PKAFILES=()
CFILES=()
OFILES=()

while [ "$#" -gt 0 ]; do
  case "$1" in
    -o) shift; OUTPUT="$1" ;;
    --platform) shift; PLATFORM="$1" ;;
    --cc) shift; CC="$1" ;;
    -R) RELEASE=1 ;;
    --) shift; CC_ARGS=("$@"); break ;;
    -*) echo "Unknown option: $1 (pass C compiler flags after --)" >&2; exit 1 ;;
    *.c) CFILES+=( "$1" ) ;;
    *.o|*.a) OFILES+=( "$1" ) ;;
    *) PKAFILES+=( "$1" ) ;;
  esac
  shift
done

if [ -z "$OUTPUT" ] || [ ${#PKAFILES[@]} -eq 0 ]; then
  echo "Usage: $0 <pka files...> [-R] [--cc compiler] [--platform target] -o <output> [-- <cc args...>]" >&2
  exit 1
fi

# --- Compiler selection ---
case "$PLATFORM" in
  win64) CC="${CC:-x86_64-w64-mingw32-gcc}" ;;
  *)     CC="${CC:-gcc}" ;;
esac

# --- Flags ---
CFLAGS="-std=c11 -Wall -Wextra -Wconversion -Wfloat-conversion \
-Wno-overlength-strings -Wno-unused-function -Wno-unused-variable -fno-strict-aliasing"

CC_NAME=$(basename "$CC")
if [[ "$CC_NAME" == clang || "$CC_NAME" == clang-* ]]; then
  CFLAGS+=" -Wno-gnu-auto-type -Wno-gnu-statement-expression -Wno-parentheses-equality"
fi

case "$PLATFORM" in
  x86_64_raw|aarch64_raw)
    CFLAGS+=" -fno-builtin -ffreestanding -nostdlib -fno-stack-protector -static"
    LIBS="${LIBS:-} -lgcc"
    if [ "$PLATFORM" = aarch64_raw ]; then
      CFLAGS+=" -mno-outline-atomics"
    fi
    ;;
  libc)
    CFLAGS+=" -D_GNU_SOURCE -Wno-unused-result"
    LIBS="${LIBS:-} -lrt" ;;
  win64)
    CFLAGS+=" -Wno-unused-result -mconsole"
    LIBS="${LIBS:-} -lws2_32 -lbcrypt" ;;
  *) echo "Unknown platform: $PLATFORM" >&2; exit 1 ;;
esac

if [ "$RELEASE" = 1 ]; then
  CFLAGS+=" -O3 -ffast-math -fno-finite-math-only -funroll-loops -ftree-vectorize \
-finline-functions -ffunction-sections -fdata-sections -flto -s \
-fno-ident -Wl,--gc-sections,--strip-all"
  case "$PLATFORM" in
    aarch64_raw) CFLAGS+=" -mcpu=native" ;;
    x86_64_raw|libc) CFLAGS+=" -march=native -mtune=native" ;;
  esac
  if [ "$PLATFORM" = x86_64_raw ]; then
    CFLAGS+=" -mshstk"
  fi
  case "$PLATFORM" in
    x86_64_raw|aarch64_raw)
      CFLAGS+=" -fno-asynchronous-unwind-tables -fno-unwind-tables" ;;
  esac
  if [ "$PLATFORM" != win64 ]; then
    CFLAGS+=" -Wl,--build-id=none"
    if [[ "$CC_NAME" == clang || "$CC_NAME" == clang-* ]]; then
      CFLAGS+=" -fuse-ld=lld"
      if [ "$PLATFORM" != libc ]; then
        CFLAGS+=" -fno-PIC"
      fi
    fi
  fi
fi

if [ -n "${CFLAGS_EXTRA:-}" ]; then
  CFLAGS="$CFLAGS $CFLAGS_EXTRA"
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
trap 'rm -f "$TMPFILE" "$LIBPK_C" "$TMPPKA"' EXIT

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
  if [ ${#CFILES[@]} -gt 0 ]; then
    cat "${CFILES[@]}"
  fi
  # Compile each lib module with -N <modname>
  for pka in "${LIBPK_MODULES[@]}"; do
    modname=$(basename "$(dirname "$pka")")
    {
      printf '%s\n' "-N $modname"
      cat "$pka"
    } > "$TMPPKA"
    "$PIKA2C" < "$TMPPKA"
  done
  # Compile user .pka files with lib headers for types/defaults/symbols
  {
    for pka in "${LIBPK_MODULES[@]}"; do
      modname=$(basename "$(dirname "$pka")")
      {
        printf '%s\n' "-H"
        printf '%s\n' "-N $modname"
        cat "$pka"
      } > "$TMPPKA"
      "$PIKA2C" < "$TMPPKA"
    done
    cat "${PKAFILES[@]}"
  } | "$PIKA2C"
  cat "$LIBPK_C"
  cat "$RT_FILE"
} > "$TMPFILE"

# --- Compile ---
start_ms=$(date +%s%3N)
"$CC" $CFLAGS -o "$OUTPUT" "$TMPFILE" "${OFILES[@]}" ${LIBS:-} "${CC_ARGS[@]}"
end_ms=$(date +%s%3N)
duration=$((end_ms - start_ms))

echo "Built $OUTPUT"
echo "${duration} ms"
