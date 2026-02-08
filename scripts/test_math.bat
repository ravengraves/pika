@echo off
cd /d "%~dp0\.."

if not exist build\test mkdir build\test
type stdlib\math.pka | bin\pika2c.exe > build\test\math.c
type test\math_test.c >> build\test\math.c
gcc.exe -std=c11 -Wall -Wextra -O3 -fno-strict-aliasing ^
    -O3 -ffast-math -funroll-loops -ftree-vectorize ^
    -finline-functions ^
    build\test\math.c -lm -o build\test\math_test.exe
if errorlevel 1 exit /b 1
build\test\math_test.exe
