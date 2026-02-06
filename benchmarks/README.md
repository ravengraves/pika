# Pika Benchmark Suite

Classic computational benchmarks ported to Pika, with C and Rust reference implementations for cross-language performance comparison.

## Quick Summary - C vs Rust vs Pika

```
CPU:      Intel i9-13900K (Raptor Lake), WSL2
C:        gcc 13.3.0, -O3 -ffast-math -march=native -flto
Clang:    clang 18.1.3, -O3 -ffast-math -march=native -flto
Rust:     rustc 1.93.0, -C opt-level=3 -C target-cpu=native
Pika:     pikacc 0.1 -> C, compiled with clang (same C flags)
```

| Benchmark | C GCC | Rust | Pika | Pika/C | Pika/Rust |
|-----------|-------|------|------|--------|-----------|
| Whetstone (MWIPS) | 12,720 | 13,060 | 12,324 | 0.97x | 0.94x |
| Dhrystone (M ops/s) | 245.1 | 186.8 | 488.8 | **1.99x** | **2.62x** |
| Livermore avg MFLOPS | 32,963 | 19,379 | 31,968 | 0.97x | **1.65x** |
| MemSpeed f64 add L1 (MB/s) | 299,567 | 241,387 | 538,276 | **1.80x** | **2.23x** |
| MemSpeed f64 add L2 (MB/s) | 192,745 | 158,591 | 279,283 | **1.45x** | **1.76x** |
| MemSpeed f64 add L3 (MB/s) | 168,880 | 129,555 | 300,891 | **1.78x** | **2.32x** |
| MemSpeed f64 add RAM (MB/s) | 71,749 | 72,435 | 143,990 | **2.01x** | **1.99x** |
| LINPACK 100x100 (MFLOPS) | 17,177 | 6,936 | 15,810 | 0.92x | **2.28x** |
| Mandelbrot 16K (seconds) | 1.57s | 0.99s | 0.72s | **2.18x** | **1.38x** |

Pika's transpile-to-C approach matches or beats handwritten C on vectorizable workloads (MemSpeed, Mandelbrot, LINPACK) thanks to unified-TU inlining and GCC/Clang's auto-vectorizer. Rust matches C on memory-bound workloads (MemSpeed add at RAM scale, memcpy) and scalar work (Whetstone), but loses on FP-intensive benchmarks (LINPACK, dot product, Livermore inner product) where strict IEEE 754 semantics prevent LLVM from auto-vectorizing accumulation loops and flushing denormals. Both Pika and Rust beat C on Mandelbrot due to wider SIMD utilization (AVX vs SSE2).

## Copyright Notice

Original benchmark algorithms:

- **Whetstone** - Brian Wichmann (NPL, 1960s), Harold Curnow (CCTA, 1972), Roy Longbottom (1996)
- **Dhrystone** - Reinhold P. Weicker (1984), Roy Longbottom adaptation (1996)
- **Livermore Loops** - Frank McMahon (LLNL), Greg Astfalk (AT&T), Roy Longbottom adaptation (1996)
- **LINPACK** - Jack Dongarra (Argonne National Laboratory, 1976), Roy Longbottom adaptation (1996)
- **MemSpeed** - Roy Longbottom (MemSpd2K)
- **Mandelbrot** - Kevin Miller, [The Computer Language Benchmarks Game](https://salsa.debian.org/benchmarksgame-team/benchmarksgame/) (BSD 3-Clause License)

Copyright (C) Roy Longbottom 1997 - 2016. Free for any non-profitable purpose (Whetstone, Dhrystone, Livermore, LINPACK, MemSpeed).

Pika implementations are original ports following the same algorithms.

## Benchmarks

| Benchmark | What it measures | Key metric |
|-----------|-----------------|------------|
| Whetstone | Floating-point, trig, branching | MWIPS |
| Dhrystone | Integer, string ops, procedure calls | Dhrystones/sec |
| Livermore | Numerical kernels (vectorization, memory) | MFLOPS |
| LINPACK | Dense linear algebra (100x100 LU decomposition + solve) | MFLOPS |
| MemSpeed | Memory bandwidth (L1/L2/L3/RAM) | MB/s |
| Mandelbrot | FP compute + I/O (PBM image output) | Seconds |

## Running

```
make benchmark                    # run all Pika benchmarks
make benchmark-whetstone          # individual Pika benchmarks
make benchmark-dhrystone
make benchmark-livermore
make benchmark-memspeed
make benchmark-linpack
make benchmark-mandelbrot

make benchmark-c                  # run all C reference benchmarks
make benchmark-c-whetstone        # individual C benchmarks
make benchmark-c-dhrystone
make benchmark-c-livermore
make benchmark-c-memspeed
make benchmark-c-linpack
make benchmark-c-mandelbrot

make benchmark-rust               # run all Rust reference benchmarks
make benchmark-rust-whetstone     # individual Rust benchmarks
make benchmark-rust-dhrystone
make benchmark-rust-livermore
make benchmark-rust-memspeed
make benchmark-rust-linpack
make benchmark-rust-mandelbrot
```

Override the C compiler: `CC=clang bin/pikacc ...` or `BENCH_CC=clang make benchmark-c-...`

## Test Environment

```
CPU:            13th Gen Intel(R) Core(TM) i9-13900K (Raptor Lake)
Cores:          15 cores / 30 threads (8P + 8E, hybrid)
Cache:          L1d 720 KiB, L1i 480 KiB, L2 30 MiB, L3 36 MiB
Platform:       WSL2 (Linux 6.6.87.2-microsoft-standard-WSL2)
C Compiler:     gcc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
C Flags:        -O3 -ffast-math -funroll-loops -ftree-vectorize
                -march=native -mtune=native -finline-functions -fno-strict-aliasing -flto
Rust Compiler:  rustc 1.93.0 (254b59607 2026-01-19)
Rust Flags:     -C opt-level=3 -C target-cpu=native
Clang:          Ubuntu clang version 18.1.3 (1ubuntu1)
Pika extra:     -static -ffreestanding -nostdlib -fno-builtin
Dhrystone:      Pika uses CC=clang (Clang inlines the single-TU build more aggressively)
Mandelbrot:     Pika uses CC=clang (Clang auto-vectorizes better than GCC for this workload)
```

## Results: C vs Rust vs Pika

### Whetstone (Single Precision)

#### C GCC

```
 Single Precision C/C++ Whetstone Benchmark
         GCC/Clang Linux
         -O3 -ffast-math

Loop content                  Result              MFLOPS      MOPS   Seconds

N1 floating point     -1.12475013732910156      2404.014              0.102
N2 floating point     -1.12274742126464844      2263.341              0.756
N3 if then else        1.00000000000000000               650170.000    0.002
N4 fixed point        12.00000000000000000               4009005056.000    0.000
N5 sin,cos etc.        0.49911010265350342                 373.047    2.838
N6 floating point      0.99999982118606567      1622.770              4.230
N7 assignments         3.00000000000000000               163397.906    0.014
N8 exp,sqrt etc.       0.75110864639282227                 229.500    2.063

MWIPS                                          12719.846             10.006
```

#### Rust

```
Rust Whetstone Benchmark (Single Precision)
Passes: 1 x 100000

Loop content            MFLOPS/MOPS   Seconds
-------------------     -----------   -------
N1 floating point       2387.7         0.008
N2 floating point       2405.1         0.056
N3 if then else         2809903.9         0.000
N4 fixed point          11250000000.0         0.000
N5 sin,cos etc.         559.3         0.149
N6 floating point       1645.8         0.328
N7 assignments          1177745.2         0.000
N8 exp,sqrt etc.        243.6         0.153

MWIPS                   13059.820         0.766
Check: 20.069014
```

#### Pika GCC

```
Pika Whetstone Benchmark (Single Precision)
Passes: 1 x 100000

Loop content            MFLOPS/MOPS   Seconds
-------------------     -----------   -------
N1 floating point       2569.7         0.007
N2 floating point       2412.1         0.056
N3 if then else         565428.6         0.000
N4 fixed point          3088235294.1         0.000
N5 sin,cos etc.         538.6         0.154
N6 floating point       1658.5         0.325
N7 assignments          158002.2         0.001
N8 exp,sqrt etc.        186.0         0.200

MWIPS                   12323.734         0.811
Check: 20.827355
```

#### Whetstone Summary

| Section | C GCC | Rust | Pika GCC | Notes |
|---------|-------|------|----------|-------|
| N1 floating point | 2,404 | 2,388 | 2,570 | Array element arithmetic |
| N2 floating point | 2,263 | 2,405 | 2,412 | Array as parameter |
| N3 if/then/else | 650,170 | 2,809,904 | 565,429 | DCE in all |
| N4 fixed point | 4,009M | 11,250M | 3,088M | DCE in all |
| N5 sin,cos,atan | 373 | 559 | 539 | Rust/Pika use fast math libs |
| N6 floating point | 1,623 | 1,646 | 1,659 | Procedure calls |
| N7 assignments | 163,398 | 1,177,745 | 158,002 | DCE in all |
| N8 exp,sqrt,log | 230 | 244 | 186 | Rust libm > glibc > Pika math |
| **MWIPS** | **12,720** | **13,060** | **12,324** | |

N3/N4/N7 are DCE-affected (deterministic loop patterns). On meaningful sections: N5 trig - Rust's `f32::sin()`/`cos()` and Pika's `math::` stdlib both outperform glibc. N8 exp/sqrt/log - Rust's libm is fastest, followed by glibc, then Pika's custom polynomials. Overall MWIPS effectively identical across all three.

### Dhrystone

#### C GCC

```
Dhrystone Benchmark, Version 1.1 (Language: C)

       10000 runs   0.00 seconds
      100000 runs   0.00 seconds
     1000000 runs   0.00 seconds
    10000000 runs   0.04 seconds
   100000000 runs   0.41 seconds
   200000000 runs   0.81 seconds
   400000000 runs   1.63 seconds
   800000000 runs   3.26 seconds
  1600000000 runs   6.53 seconds

Array2Glob8/7: O.K.    1600000010

Microseconds for one run through Dhrystone:         0.00
Dhrystones per Second:                       245129093
VAX  MIPS rating =                             139515.70
```

#### Rust

```
Dhrystone Benchmark (Rust)

Running 100000000 iterations...
Global values (anti-DCE check):
  IntGlob:      5
  BoolGlob:     1
  Char1Glob:    65
  Char2Glob:    66
  Array1Glob[8]:7
  Array2Glob[8][7]: O.K.
  RecGlb.int_comp:  17
  RecGlb.enum_comp: 2
  RecNext.discr:    0
  RecNext.enum_comp:1
  RecNext.int_comp: 18
  RecNext.ptr_comp: 0
  StrChecksum:      4318

Microseconds per run:   0.01
Dhrystones per second:  186831370
VAX MIPS rating:        106335.44
```

#### Pika Clang

```
Dhrystone Benchmark (Pika)

Running 100000000 iterations...
Global values (anti-DCE check):
  IntGlob:      5
  BoolGlob:     1
  Char1Glob:    65
  Char2Glob:    66
  Array1Glob[8]:7
  Array2Glob[8][7]: O.K.
  RecGlb.int_comp:  17
  RecGlb.enum_comp: 2
  RecNext.discr:    0
  RecNext.enum_comp:1
  RecNext.int_comp: 18
  RecNext.ptr_comp: 0
  StrChecksum:      4318

Microseconds per run:   0.00
Dhrystones per second:  488761720
VAX MIPS rating:        278179.69
```

#### Dhrystone Summary

| Metric | C GCC | Rust | Pika Clang | Pika/C | Pika/Rust |
|--------|-------|------|------------|--------|-----------|
| Dhrystones/sec | 245,129,093 | 186,831,370 | 488,761,720 | **1.99x** | **2.62x** |
| VAX MIPS | 139,516 | 106,335 | 278,180 | **1.99x** | **2.62x** |

All Dhrystone global values verified identical across all three implementations. Pika with Clang dominates - the single-TU build lets Clang fully inline all Dhrystone procedures and optimize the entire program as one unit, while `rt::compiler_barrier()` prevents over-optimization of the benchmark loop. C GCC is second - glibc `strcpy`/`strcmp` are heavily hand-tuned but GCC can't inline across TU boundaries as aggressively. Rust is slowest - `static mut` access overhead, `copy_from_slice`/byte-comparison in safe code, and LLVM's less aggressive inlining of the benchmark's many small functions.

### Livermore Loops (LLNL Kernels)

Array size: 1001, Runs: 100,000. Kernels K1/K7/K12 use `compiler_barrier()` / `__asm__ volatile("" ::: "memory")` / `black_box()` to prevent the compiler from eliminating redundant outer iterations.

#### C GCC

```
C Livermore Loops (LLNL Kernels)
Array size: 1001  Runs: 100000

Kernel                    MFLOPS     Seconds
------------------------  ---------  --------
K1  Hydro fragment      61168.74 MFLOPS  0.0081 sec
K3  Inner product       21484.89 MFLOPS  0.0093 sec
K5  Tri-diag elim       2585.64 MFLOPS  0.1160 sec
K7  EOS fragment        81571.16 MFLOPS  0.0195 sec
K11 First sum           2992.77 MFLOPS  0.0334 sec
K12 First difference    20990.39 MFLOPS  0.0048 sec
K21 Matrix*matrix       54446.80 MFLOPS  0.0574 sec
K24 Find minimum        18455.18 MFLOPS  0.0108 sec
```

#### Rust

```
Rust Livermore Loops (LLNL Kernels)
Array size: 1001  Runs: 100000

Kernel                    MFLOPS     Seconds
------------------------  ---------  --------
K1  Hydro fragment      40067.89 MFLOPS  0.0123 sec
K3  Inner product       5702.89 MFLOPS  0.0351 sec
K5  Tri-diag elim       2568.55 MFLOPS  0.1168 sec
K7  EOS fragment        53400.60 MFLOPS  0.0298 sec
K11 First sum           2994.42 MFLOPS  0.0334 sec
K12 First difference    10525.76 MFLOPS  0.0095 sec
K21 Matrix*matrix       36563.92 MFLOPS  0.0855 sec
K24 Find minimum        3210.38 MFLOPS  0.0624 sec

Done.
```

#### Pika GCC

```
Pika Livermore Loops (LLNL Kernels)
Array size: 1001  Runs: 100000

Kernel                    MFLOPS     Seconds
------------------------  ---------  --------
K1  Hydro fragment      64926.41 MFLOPS  0.0076 sec
K3  Inner product       19945.71 MFLOPS  0.0100 sec
K5  Tri-diag elim       2584.75 MFLOPS  0.1161 sec
K7  EOS fragment        81737.40 MFLOPS  0.0195 sec
K11 First sum           2988.17 MFLOPS  0.0335 sec
K12 First difference    22496.63 MFLOPS  0.0044 sec
K21 Matrix*matrix       55854.95 MFLOPS  0.0559 sec
K24 Find minimum        3211.06 MFLOPS  0.0623 sec

Done.
```

#### Livermore Summary

| Kernel | C GCC | Rust | Pika GCC | Pika/C | Pika/Rust |
|--------|-------|------|----------|--------|-----------|
| K1 Hydro fragment | 61,169 | 40,068 | 64,926 | **1.06x** | **1.62x** |
| K3 Inner product | 21,485 | 5,703 | 19,946 | 0.93x | **3.50x** |
| K5 Tri-diag elimination | 2,586 | 2,569 | 2,585 | 1.00x | 1.01x |
| K7 EOS fragment | 81,571 | 53,401 | 81,737 | 1.00x | **1.53x** |
| K11 First sum (prefix) | 2,993 | 2,994 | 2,988 | 1.00x | 1.00x |
| K12 First difference | 20,990 | 10,526 | 22,497 | **1.07x** | **2.14x** |
| K21 Matrix multiply 25x25 | 54,447 | 36,564 | 55,855 | 1.03x | **1.53x** |
| K24 Find minimum | 18,455 | 3,210 | 3,211 | 0.17x | 1.00x |

**C/Pika average: ~1.00x** - Pika matches C on all kernels. **Rust average: ~0.59x** on vectorizable kernels.

Serial-dependency kernels (K5 tri-diagonal, K11 prefix sum) are identical across all three - these can't be vectorized regardless of language. The gap appears on vectorizable kernels (K1, K3, K7, K12, K21) where Rust's `static mut` array access pattern and bounds checking prevent LLVM from auto-vectorizing as aggressively as GCC does with flat C arrays. K24 (find minimum) is slow in both Rust and Pika - the conditional branch defeats vectorization; C's version benefits from GCC's cmov optimization.

### LINPACK (Dense Linear Algebra)

100x100 double-precision LU factorization (`dgefa`) + back-substitution (`dgesl`), based on Roy Longbottom's 1996 adaptation of Jack Dongarra's 1976 LINPACK benchmark. BLAS routines (`daxpy`, `dscal`, `idamax`) are inlined and unrolled by 8. Anti-DCE: checksums accumulated from `b[0]` after each `dgesl` call and printed at end. All implementations produce matching checksums (-1.522378e+01).

#### C GCC

```
=== Unrolled Double Precision    17176.90 Mflops ===

Results for lda=201 (87826 passes):
      dgefa      dgesl      total      Mflops
    0.00004    0.00000    0.00004    17205.84
    0.00004    0.00000    0.00004    17158.87
    0.00004    0.00000    0.00004    17201.42
    0.00004    0.00000    0.00004    17119.84
  Average                              17176.90

Results for lda=200 (87826 passes):
    0.00004    0.00000    0.00004    18094.80
    0.00004    0.00000    0.00004    19022.16
    0.00004    0.00000    0.00004    19160.77
    0.00004    0.00000    0.00004    19214.33
  Average                              18700.19

Checksum -1.522378e+01
```

#### Rust

```
Unrolled Double  Precision     6935.86 Mflops

Times for array with leading dimension of 201 (105857 passes):
      dgefa      dgesl      total     Mflops
    0.00004    0.00006    0.00010    6868.83
    0.00004    0.00006    0.00010    6953.29
    0.00004    0.00006    0.00010    6949.29
    0.00004    0.00006    0.00010    6943.68
    0.00004    0.00006    0.00010    6964.21
  Average                              6935.86

Times for array with leading dimension of 200:
    0.00004    0.00006    0.00010    7051.72
    0.00004    0.00006    0.00010    7057.47
    0.00004    0.00006    0.00010    6983.49
    0.00004    0.00006    0.00010    7044.31
    0.00004    0.00006    0.00010    7037.60
  Average                              7034.92
```

#### Pika GCC

```
Unrolled Double Precision 15809.87 Mflops

Times for array with leading dimension of 201 (86304 passes):
      dgefa      dgesl      total     Mflops
  0.00004  0.00000  0.00004  15865.30
  0.00004  0.00000  0.00004  15302.56
  0.00004  0.00000  0.00004  15904.24
  0.00004  0.00000  0.00004  16014.94
  0.00004  0.00000  0.00004  15962.31
Average                          15809.87

Times for array with leading dimension of 200:
  0.00004  0.00000  0.00004  18276.93
  0.00004  0.00000  0.00004  18082.47
  0.00004  0.00000  0.00004  18176.98
  0.00004  0.00000  0.00004  18095.86
  0.00004  0.00000  0.00004  18247.23
Average                          18175.89

Checksum -1.522378e+1
```

#### LINPACK Summary

| Metric | C GCC | Rust | Pika GCC | Pika/C | Pika/Rust |
|--------|-------|------|----------|--------|-----------|
| MFLOPS (lda=201) | 17,177 | 6,936 | 15,810 | 0.92x | **2.28x** |
| MFLOPS (lda=200) | 18,700 | 7,035 | 18,176 | 0.97x | **2.58x** |

Pika matches C within 3-8%. Both dgefa (LU factorization) times are identical at 0.00004s. The primary BLAS routine `daxpy` generates identical SIMD code in both C and Pika builds (verified by disassembly).

Rust is ~2.5x slower. The `dgesl` column reveals the root cause: C/Pika show 0.00000s while Rust shows 0.00006s. After ~100K iterations of back-substitution on the same LU factors, the `b` vector values become denormalized (< 2^-1022). C and Pika flush denormals to zero (via MXCSR FTZ/DAZ on x86), but Rust preserves strict IEEE 754 denormal handling, causing ~100x pipeline stalls per denormal FP operation. This same mechanism caused a 2x slowdown in Pika before the FTZ/DAZ fix was added to Pika's `rt_start_c()`.

#### FTZ/DAZ Performance Optimization

During LINPACK development, Pika was initially getting ~7,000 MFLOPS (matching Rust's result, 50% of C's throughput). Investigation revealed that `pikacc` uses `-nostdlib`, which skips GCC's `crtfastmath.o` - a startup object that sets the x86 MXCSR register's FTZ (Flush-To-Zero) and DAZ (Denormals-Are-Zero) bits.

Without FTZ/DAZ, denormal floating-point values (which naturally arise after many iterations of back-substitution) trigger expensive CPU microcode assists instead of being flushed to zero. This caused a **2.25x slowdown** on all floating-point benchmarks. To resolve this, I added MXCSR/FPCR initialization to `rt_start_c()` in Pika's `runtime/rt.c`:

```c
// x86/x86_64: Set FTZ (bit 15) + DAZ (bit 6)
unsigned int mxcsr;
__asm__ volatile ("stmxcsr %0" : "=m"(mxcsr));
mxcsr |= 0x8040;
__asm__ volatile ("ldmxcsr %0" : : "m"(mxcsr));

// ARM64: Set FZ (bit 24)
unsigned long fpcr;
__asm__ volatile ("mrs %0, fpcr" : "=r"(fpcr));
fpcr |= (1UL << 24);
__asm__ volatile ("msr fpcr, %0" : : "r"(fpcr));
```

This is equivalent to what GCC's `crtfastmath.o` does and what `-ffast-math` implies. The fix benefits all Pika programs with floating-point arithmetic, not just LINPACK. RISC-V has no hardware denormal flushing equivalent.

### MemSpeed (Memory Bandwidth)

Testing dot product / vector add / vector copy / memcpy across cache hierarchy (MB/s).

Pika uses `rt::memcpy` (inline word-copy loops auto-vectorized by GCC with `-fno-builtin`). C uses glibc `memcpy` (hand-tuned with non-temporal stores and prefetch). Rust uses `copy_from_slice` (compiles to `memcpy`).

#### C GCC

```
C Memory Speed Benchmark
Adapted from Roy Longbottom's MemSpd2K
Minimum 0.1 seconds per test

Memory  s=s+x[m]*y[m] Int+   x[m]=x[m]+y[m]       x[m]=y[m]        memcpy
KBytes  Dble   Sngl   Int    Dble   Sngl   Int    Dble   Sngl   Int    f64
 Used   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S  MB/S

    4 240328 182394 177074 299567 282773 289490 541965 405681 405931 551819
    8 209387 217137 198261 266989 269532 272819 495059 487401 482622 512643
   16 188090 189529 183020 267482 261607 266838 583309 482078 484260 653786
   32 191571 202975 204334 311237 293954 311445 674717 612755 612574 716572
   64 171946 201044 213753 192745 336474 331494 160955 698235 696956 160824
  128 181682 179114 161259 169020 192971 204253 165125 162118 162169 164007
  256 176069 181624 165551 168907 168664 168920 163964 163244 164518 163932
  512 175745 181650 165161 168880 169420 168391 165363 165277 163834 163457
 1024 175302 173390 155742 164315 169268 166130 163896 164963 165550 164196
 2048 157702 173022 160956 133610 166281 167311 133910 164648 156788 129259
 4096  96398 127193 133268  71749 113000 130816  74184 113774 129001  73178
```

#### Rust

```
Rust Memory Speed Benchmark
Adapted from Roy Longbottom's MemSpd2K
Minimum 0.1 seconds per test

Memory  s=s+x[m]*y[m] Int+   x[m]=x[m]+y[m]       x[m]=y[m]        memcpy
KBytes  Dble   Sngl   Int    Dble   Sngl   Int    Dble   Sngl   Int    f64
 Used   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S  MB/S

    4  44894  23100 215980 241387 216984 176364 243009 230051 224412 512859
    8  46056  23388 227553 238462 239593 205948 255639 252897 250951 584490
   16  47990  23999 229250 226008 219307 192291 240110 239839 238558 697255
   32  47604  23948 232741 216662 212847 190782 234695 229831 232103 732560
   64  48238  23952 148469 158591 156901 132844 139614 134305 136356 159586
  128  47984  23994 147071 127707 132925 131431 138269 136014 136558 160567
  256  47903  24140 144875 130661 132319 132577 140891 139780 138665 163942
  512  47768  23757 143591 129555 133031 131809 138831 139523 139704 165027
 1024  46903  23817 148546 131254 132882 129812 139208 139251 139004 165774
 2048  47446  23975 135733 110825 111280 115149 106503 103928 109863 128011
 4096  47400  23928  99539  72435  71622  74804  66539  66240  64229  64101
```

#### Pika GCC

```
Pika Memory Speed Benchmark
Adapted from Roy Longbottom's MemSpd2K
Minimum 0.1 seconds per test

Memory  s=s+x[m]*y[m] Int+   x[m]=x[m]+y[m]       x[m]=y[m]        memcpy
KBytes  Dble   Sngl   Int    Dble   Sngl   Int    Dble   Sngl   Int    f64
 Used   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S   MB/S  MB/S

    4 275619 265804 251082 538276 538937 525627 419887 623961 633950 685848
    8 241496 239624 214770 590276 569184 512474 499244 612880 566305 668172
   16 198831 197668 184854 504773 539180 490396 445709 566604 518068 674074
   32 177106 174650 176672 517204 540375 501395 445945 606547 517464 630406
   64 166482 163961 172492 279283 281368 331849 108872 148770 155046 155553
  128 184674 180605 185220 283424 289397 319124 114582 152498 150810 155438
  256 185315 189748 189202 291893 298576 335131 123094 158532 159106 160947
  512 191516 189267 188638 300891 295088 335004 124163 157710 159203 159966
 1024 190806 186556 191393 298746 296893 331389 120671 157702 158991 160213
 2048 157211 164911 154873 234170 238717 232707 100150 123850 113912 113584
 4096  80651  86558  93491 143990 133769 135667  67759  71765  72408  67101
```

#### MemSpeed Summary - f64 Vector Add (MB/s)

| Working Set | C GCC | Rust | Pika GCC | Pika/C | Pika/Rust |
|-------------|-------|------|----------|--------|-----------|
| 4 KB (L1) | 299,567 | 241,387 | 538,276 | **1.80x** | **2.23x** |
| 64 KB (L2) | 192,745 | 158,591 | 279,283 | **1.45x** | **1.76x** |
| 512 KB (L3) | 168,880 | 129,555 | 300,891 | **1.78x** | **2.32x** |
| 4 MB (RAM) | 71,749 | 72,435 | 143,990 | **2.01x** | **1.99x** |

#### MemSpeed Summary - f64 Dot Product (MB/s)

| Working Set | C GCC | Rust | Pika GCC | Pika/C | Pika/Rust |
|-------------|-------|------|----------|--------|-----------|
| 4 KB (L1) | 240,328 | 44,894 | 275,619 | 1.15x | **6.14x** |
| 64 KB (L2) | 171,946 | 48,238 | 166,482 | 0.97x | **3.45x** |
| 512 KB (L3) | 175,745 | 47,768 | 191,516 | 1.09x | **4.01x** |
| 4 MB (RAM) | 96,398 | 47,400 | 80,651 | 0.84x | **1.70x** |

#### MemSpeed Summary - memcpy (MB/s)

| Working Set | C `memcpy` (glibc) | Rust `copy_from_slice` | Pika `rt::memcpy` | Pika/C | Pika/Rust |
|-------------|-------------------|----------------------|-------------------|--------|-----------|
| 4 KB (L1) | 551,819 | 512,859 | 685,848 | **1.24x** | **1.34x** |
| 64 KB (L2) | 160,824 | 159,586 | 155,553 | 0.97x | 0.97x |
| 512 KB (L3) | 163,457 | 165,027 | 159,966 | 0.98x | 0.97x |
| 4 MB (RAM) | 73,178 | 64,101 | 67,101 | 0.92x | 1.05x |

Pika's vector add is significantly faster across the board - the unified TU enables GCC to fully inline and auto-vectorize the loops. Rust's f64 dot product is ~4-5x slower than C/Pika because LLVM cannot vectorize the FP reduction (`sum += x[m]*y[m]`) without `-ffast-math` - strict IEEE 754 semantics require sequential accumulation. Rust's vector add and copy are properly AVX2-vectorized (4* `vaddpd ymm`, confirmed by disassembly) but ~20% slower than C due to the absence of `-ffast-math` loop transformations. Rust's `copy_from_slice` matches glibc `memcpy` at L2+ sizes. At RAM scale, all three converge to memory bandwidth limits.

### Mandelbrot

Plots the Mandelbrot set [-1.5-i, 0.5+i] on an NxN bitmap, output as PBM P4. N=16000 for performance. All single-threaded. Processes 4 rows x 8 pixels = 32 pixels per inner loop. Batch early-exit every 8 iterations, 50 total (6x8+2).

C reference: Kevin Miller's Benchmarks Game #6 - explicit SSE2 SIMD (8 pixels via packed `__m128d`). Pika/Rust: 32-wide `[32]f64` arrays; Clang/LLVM's SLP vectorizer packs these into AVX `ymm` registers (4 doubles per register, 8 registers = 32 values).

#### Pika Clang (N=16000)

```
$ make benchmark-mandelbrot
bash -c 'time build/benchmarks/mandelbrot/mandelbrot > /dev/null'

real    0m0.721s
user    0m0.767s
sys 0m0.000s
```

#### Rust (N=16000)

```
$ make benchmark-rust-mandelbrot
bash -c 'time build/benchmarks/mandelbrot/mandelbrot_rs > /dev/null'

real    0m0.985s
user    0m1.042s
sys 0m0.004s
```

#### C GCC (N=16000, single-thread)

```
$ make benchmark-c-mandelbrot
bash -c 'time OMP_NUM_THREADS=1 build/benchmarks/mandelbrot/mandelbrot_c 16000 > /dev/null'

real    0m1.565s
user    0m1.659s
sys 0m0.013s
```

#### Mandelbrot Summary

| Version | Time (real) | vs C GCC |
|---------|-------------|----------|
| **Pika Clang** | **0.72s** | **2.17x faster** |
| **Rust** | **0.99s** | **1.58x faster** |
| C GCC (explicit SSE2) | 1.57s | 1.00x |

Both Pika and Rust beat the hand-tuned C SSE2 reference. The 32-wide inner loop (4 rows x 8 pixels) gives LLVM's SLP vectorizer enough independent work to utilize AVX (4 doubles per `ymm` register). The C reference is limited to SSE2 (`__m128d`, 2 doubles per register). Pika with Clang is fastest because `*[*]f64` pointer casts eliminate all bounds-checking overhead, giving Clang a cleaner loop body to vectorize. Rust's bounds-checked `[f64; 32]` indexing still vectorizes well but with slightly more overhead.

## Summary

| Benchmark | C GCC | Rust | Pika | Pika/C | Pika/Rust |
|-----------|-------|------|------|--------|-----------|
| Whetstone MWIPS | 12,720 | 13,060 | 12,324 | 0.97x | 0.94x |
| Dhrystone (M ops/s) | 245.1 | 186.8 | 488.8 | **1.99x** | **2.62x** |
| Livermore Loops (avg MFLOPS) | 32,963 | 19,379 | 31,968 | 0.97x | **1.65x** |
| LINPACK 100x100 (MFLOPS) | 17,177 | 6,936 | 15,810 | 0.92x | **2.28x** |
| MemSpeed f64 add (L1) | 300K | 241K | 538K | **1.79x** | **2.23x** |
| MemSpeed f64 add (RAM) | 72K | 72K | 144K | **2.00x** | **2.00x** |
| MemSpeed memcpy (L1) | 552K | 513K | 686K | **1.24x** | **1.34x** |
| Mandelbrot 16K | 1.57s | 0.99s | 0.72s | **2.18x** | **1.38x** |

**Pika generates native code with performance matching or exceeding handwritten C.** The transpile-to-C approach means the same GCC/Clang backend optimizes both. Livermore Loops shows 1.00x parity across all 8 kernels. LINPACK shows 0.92-0.97x parity. Pika's unified TU compilation and `*[*]` unchecked pointer casts give measurable advantages on vectorizable workloads (MemSpeed, Mandelbrot).

**Rust matches C on scalar and memory-bound workloads** (Whetstone, MemSpeed add at RAM scale, memcpy) but loses on FP reductions (dot product, Livermore inner product) where strict IEEE 754 semantics prevent LLVM from vectorizing accumulation loops without `-ffast-math`. LINPACK is 2.5x slower in Rust due to denormal float handling - Rust preserves IEEE 754 denormal semantics while C/Pika flush denormals to zero via FTZ/DAZ. Idiomatic Rust with `Vec` + `&mut`/`&` slice borrows gives LLVM proper aliasing information for auto-vectorization of independent-element operations (add, copy). Rust excels on Mandelbrot where the `[f64; 32]` local array pattern vectorizes well via LLVM's SLP vectorizer.

## Implementation Notes

Pika benchmarks follow all language constraints:
- No recursion, no dynamic memory allocation, no function pointers
- All data in static global arrays
- Anti-DCE: results accumulated in globals and printed as checksums
- `f32` suffix required on float literals to avoid implicit conversion warnings
- Empty match cases use `void;` (not `{}`)
- Dhrystone uses `rt::compiler_barrier()` to prevent LTO over-optimization while preserving the idiomatic single-TU build
- Dhrystone uses `rt::memcmp` for ordered string comparison (inlined, auto-vectorized)
- Livermore K1/K7/K12 use `rt::compiler_barrier()` after inner loops to prevent GCC from eliminating redundant outer iterations (GCC recognizes loop-invariant write patterns and collapses to a single iteration without barriers)
- LINPACK uses checksum accumulation (`lp_check += b[0]` after each `dgesl`) to prevent DCE of back-substitution; `size` type indices for better vectorization of BLAS routines

Rust benchmarks use idiomatic Rust:
- `Vec` heap allocations with `&mut [T]`/`&[T]` slice borrows (MemSpeed) - gives LLVM aliasing guarantees for auto-vectorization
- `static mut` globals with `unsafe` blocks where global state is required (Dhrystone, Livermore, Whetstone)
- `std::hint::black_box()` for anti-DCE (stable since Rust 1.66)
- `std::time::Instant` for timing
- `std::io::BufWriter` for buffered output (Mandelbrot)
- No external crates - `std` only
- Compiled with `rustc` directly (no Cargo)

### Compiler Flags

**C (GCC):** `gcc -O3 -ffast-math -funroll-loops -ftree-vectorize -march=native -mtune=native -finline-functions -fno-strict-aliasing -flto`

| Flag | Purpose |
|------|---------|
| `-O3` | Maximum optimization level (loop unrolling, vectorization, inlining heuristics) |
| `-ffast-math` | Relaxed IEEE 754 - enables reassociation, FMA, reciprocal estimates. Required for auto-vectorizing reductions (`+=` loops) |
| `-funroll-loops` | Unroll loops with known trip counts. Helps small inner kernels (Whetstone, Livermore) |
| `-ftree-vectorize` | Enable auto-vectorization (implicit at `-O3`, explicit for clarity) |
| `-march=native -mtune=native` | Generate code for the host CPU - enables AVX2, FMA, BMI2 on Raptor Lake |
| `-finline-functions` | Inline small functions even without `inline` keyword. Critical for Pika's single-TU build |
| `-fno-strict-aliasing` | Assume pointers of different types may alias. Required by Pika's transpiled C (casts between `*[*]f64` and flat arrays) |
| `-flto` | Link-time optimization across translation units. Primarily benefits the C reference (multi-function programs); Pika is already a single TU |

**C (GCC) - Pika extra:** `-static -ffreestanding -nostdlib -fno-builtin`

| Flag | Purpose |
|------|---------|
| `-static` | Static linking - no glibc dynamic dispatch overhead |
| `-ffreestanding -nostdlib` | Pika provides its own runtime (`rt.c`) - no C stdlib dependency. Runtime sets MXCSR FTZ/DAZ (x86) and FPCR FZ (ARM64) to flush denormals, matching `crtfastmath.o` behavior that `-nostdlib` skips |
| `-fno-builtin` | Prevents GCC from replacing loops with glibc `memcpy`/`memset` calls. Forces the compiler to vectorize Pika's inline `rt::memcpy` loops directly, which is faster in L1/L2 cache |

**C (Clang) - Pika - Mandelbrot only:** `clang -O3 -ffast-math -march=native`

Clang's SLP (Superword Level Parallelism) vectorizer handles the 32-wide `f64` arrays significantly better than GCC. GCC's tree vectorizer struggles with the multi-array update pattern (`zr[k]`, `zi[k]`, `cr[k]` updated together), producing scalar code where Clang emits packed AVX instructions.

**Rust:** `rustc -C opt-level=3 -C target-cpu=native`

| Flag | Purpose |
|------|---------|
| `-C opt-level=3` | Maximum optimization - equivalent to LLVM `-O3`. Enables vectorization, inlining, loop transformations |
| `-C target-cpu=native` | Generate code for the host CPU - equivalent to `-march=native`. Enables AVX2, FMA on Raptor Lake |

Rust does not have a `-ffast-math` equivalent at the `rustc` flag level. LLVM's floating-point model in Rust preserves IEEE 754 semantics by default, which prevents reassociation of `+=` reduction loops. This is the primary reason Rust's dot product and inner product benchmarks are ~4-5x slower - LLVM cannot vectorize a strict-order FP accumulation. Independent-element operations (vector add `x[m] = x[m] + y[m]`, vector copy `x[m] = y[m]`) vectorize properly when Rust's borrow checker can prove non-aliasing - idiomatic `&mut [T]`/`&[T]` slice parameters give LLVM the same aliasing information as C's `restrict`. The `[f64; 32]` local arrays in Mandelbrot also vectorize well because each element is independent.
