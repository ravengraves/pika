@echo off
setlocal
cd /d "%~dp0\.."

set BENCH_CC=gcc.exe
set BENCH_CFLAGS=-O3 -ffast-math -funroll-loops -ftree-vectorize -finline-functions -fno-strict-aliasing -flto

for %%b in (whetstone dhrystone livermore memspeed linpack mandelbrot) do (
    if not exist build\benchmarks\%%b mkdir build\benchmarks\%%b
)

for %%b in (whetstone dhrystone livermore memspeed linpack) do (
    echo === C %%b ===
    %BENCH_CC% %BENCH_CFLAGS% -o build\benchmarks\%%b\%%b_c.exe benchmarks\c_reference\%%b_ref.c -lm
    if errorlevel 1 exit /b 1
    build\benchmarks\%%b\%%b_c.exe
)

echo === C mandelbrot ===
%BENCH_CC% %BENCH_CFLAGS% -fno-finite-math-only -o build\benchmarks\mandelbrot\mandelbrot_c.exe benchmarks\c_reference\mandelbrot_ref.c
if errorlevel 1 exit /b 1
build\benchmarks\mandelbrot\mandelbrot_c.exe 16000 > NUL
