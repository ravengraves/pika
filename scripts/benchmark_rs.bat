@echo off
setlocal
cd /d "%~dp0\.."

set BENCH_RUSTFLAGS=-C opt-level=3 -C target-cpu=native

for %%b in (whetstone dhrystone livermore memspeed linpack mandelbrot) do (
    if not exist build\benchmarks\%%b mkdir build\benchmarks\%%b
)

for %%b in (whetstone dhrystone livermore memspeed linpack) do (
    echo === Rust %%b ===
    rustc %BENCH_RUSTFLAGS% -o build\benchmarks\%%b\%%b_rs.exe benchmarks\rust_reference\%%b_ref.rs
    if errorlevel 1 exit /b 1
    build\benchmarks\%%b\%%b_rs.exe
)

echo === Rust mandelbrot ===
rustc %BENCH_RUSTFLAGS% -o build\benchmarks\mandelbrot\mandelbrot_rs.exe benchmarks\rust_reference\mandelbrot_ref.rs
if errorlevel 1 exit /b 1
build\benchmarks\mandelbrot\mandelbrot_rs.exe > NUL
