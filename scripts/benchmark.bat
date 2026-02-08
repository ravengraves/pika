@echo off
setlocal
cd /d "%~dp0\.."

for %%b in (whetstone dhrystone mandelbrot livermore memspeed linpack) do (
    if not exist build\benchmarks\%%b mkdir build\benchmarks\%%b
)

echo === Pika whetstone ===
bin\pikacc.exe benchmarks\bench.pka benchmarks\whetstone\whetstone.pka pikabench.c -o build\benchmarks\whetstone\whetstone.exe
if errorlevel 1 exit /b 1
build\benchmarks\whetstone\whetstone.exe

echo === Pika dhrystone ===
bin\pikacc.exe benchmarks\bench.pka benchmarks\dhrystone\dhrystone.pka pikabench.c -o build\benchmarks\dhrystone\dhrystone.exe
if errorlevel 1 exit /b 1
build\benchmarks\dhrystone\dhrystone.exe

echo === Pika mandelbrot ===
bin\pikacc.exe benchmarks\bench.pka benchmarks\mandelbrot\mandelbrot.pka pikabench.c -o build\benchmarks\mandelbrot\mandelbrot.exe
if errorlevel 1 exit /b 1
build\benchmarks\mandelbrot\mandelbrot.exe > NUL

echo === Pika livermore ===
bin\pikacc.exe benchmarks\bench.pka benchmarks\livermore\livermore.pka pikabench.c -o build\benchmarks\livermore\livermore.exe
if errorlevel 1 exit /b 1
build\benchmarks\livermore\livermore.exe

echo === Pika memspeed ===
bin\pikacc.exe benchmarks\bench.pka benchmarks\memspeed\memspeed.pka pikabench.c -o build\benchmarks\memspeed\memspeed.exe
if errorlevel 1 exit /b 1
build\benchmarks\memspeed\memspeed.exe

echo === Pika linpack ===
bin\pikacc.exe benchmarks\bench.pka benchmarks\linpack\linpack.pka pikabench.c -o build\benchmarks\linpack\linpack.exe
if errorlevel 1 exit /b 1
build\benchmarks\linpack\linpack.exe
