# Magic

I found that you can get pretty close to optimal trig coefficients using evolutionary search.

So I make some quick scripts that use this for search in python.

I am using these coefficients for `stdlib/math.pka` (32-bit fp library).

## Errors 
Maximum error from an evaluation of 5000 points from the function's intended range are documented in the math.pka standard library file.

## Math Library Perfoormance
The numerical approximations using these coefficients are very performant when paired with modern compiler (clang & gcc) auto-vectorization capabilities and on-par-with or faster than standard libraries like libmvec.

Here is a comparison other math libraries (see `test/math_benchmark.c`):

**Clang Test:** 1M elements, 4x unrolled loops, best of 30 runs
```
clang -D_GNU_SOURCE -std=c11 -Wall -Wextra -fno-strict-aliasing \
		-O3 -ffast-math -ftree-vectorize -march=native -mtune=native \
		-fvectorize -fslp-vectorize -fno-math-errno \
		-finline-functions -flto -fuse-ld=lld \
		build/test/math_benchmark.c -lmvec -lm -o build/test/math_benchmark
```
| Function | libm (µs) | pika (µs) | Speedup | Status |
|----------|-----------|-----------|---------|--------|
| recip    |       138 |       154 |   0.90x | ⚠ slower |
| rsqrt    |       151 |       189 |   0.80x | ⚠ slower |
| sqrt     |       140 |       198 |   0.71x | ⚠ slower |
| exp      |      1679 |       282 |   5.96x | 🚀 FASTER |
| log      |      1562 |       283 |   5.53x | 🚀 FASTER |
| pow      |      3016 |       657 |   4.59x | 🚀 FASTER |
| sin      |      5552 |       408 |  13.59x | 🚀 FASTER |
| cos      |      5639 |       426 |  13.24x | 🚀 FASTER |
| sincos   |      6159 |       612 |  10.06x | 🚀 FASTER |
| tan      |      8885 |       545 |  16.30x | 🚀 FASTER |
| atan     |      5884 |       316 |  18.59x | 🚀 FASTER |
| asin     |      6042 |       403 |  14.98x | 🚀 FASTER |
| acos     |      5733 |       419 |  13.68x | 🚀 FASTER |
| atan2    |     15824 |       607 |  26.07x | 🚀 FASTER |

---

**Clang + libmvec Test:** 1M elements, 4x unrolled loops, best of 30 runs

```
clang -D_GNU_SOURCE -std=c11 -Wall -Wextra -fno-strict-aliasing \
		-O3 -ffast-math -ftree-vectorize -march=native -mtune=native \
		-fvectorize -fslp-vectorize -fno-math-errno -fveclib=libmvec \
		-finline-functions -flto -fuse-ld=lld \
		build/test/math_benchmark.c -lmvec -lm -o build/test/math_benchmark
```

| Function | libmvec (µs) | pika (µs) | Speedup | Status |
|----------|-----------|-----------|---------|--------|
| recip    |       132 |       148 |   0.90x | ⚠ slower |
| rsqrt    |       157 |       190 |   0.82x | ⚠ slower |
| sqrt     |       141 |       198 |   0.71x | ⚠ slower |
| exp      |       307 |       285 |   1.08x | ≈ same |
| log      |       319 |       283 |   1.13x | ✓ faster |
| pow      |      1537 |       637 |   2.41x | 🚀 FASTER |
| sin      |       321 |       402 |   0.80x | ⚠ slower |
| cos      |       320 |       422 |   0.76x | ⚠ slower |
| sincos   |       633 |       633 |   1.00x | ≈ same |
| tan      |      8945 |       543 |  16.47x | 🚀 FASTER |
| atan     |      5844 |       317 |  18.42x | 🚀 FASTER |
| asin     |      6059 |       412 |  14.70x | 🚀 FASTER |
| acos     |      5630 |       415 |  13.58x | 🚀 FASTER |
| atan2    |     15738 |       604 |  26.05x | 🚀 FASTER |

---

**GCC -ffast-math Test:** 1M elements, 4x unrolled loops, best of 30 runs
```
gcc -D_GNU_SOURCE -std=c11 -Wall -Wextra -O3 -fno-strict-aliasing \
		-O3 -ffast-math -funroll-loops -ftree-vectorize -march=native -mtune=native \
		-finline-functions -finline-limit=10000 -flto \
		build/test/math_benchmark.c -lm -o build/test/math_benchmark
```

| Function | libmvec (µs) | pika (µs) | Speedup | Status |
|----------|-----------|-----------|---------|--------|
| recip    |       119 |       116 |   1.03x | ≈ same |
| rsqrt    |       124 |       122 |   1.01x | ≈ same |
| sqrt     |       120 |       125 |   0.96x | ≈ same |
| exp      |       278 |       166 |   1.67x | 🚀 FASTER |
| log      |       314 |       187 |   1.68x | 🚀 FASTER |
| pow      |      1577 |       447 |   3.53x | 🚀 FASTER |
| sin      |       314 |       325 |   0.96x | ≈ same |
| cos      |       320 |       328 |   0.97x | ≈ same |
| sincos   |       706 |       463 |   1.52x | 🚀 FASTER |
| tan      |       370 |       418 |   0.89x | ⚠ slower |
| atan     |       340 |       224 |   1.52x | 🚀 FASTER |
| asin     |       381 |       314 |   1.21x | ✓ faster |
| acos     |       429 |       331 |   1.30x | ✓ faster |
| atan2    |       522 |       416 |   1.25x | ✓ faster |
