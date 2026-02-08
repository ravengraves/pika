@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

if not exist build\test\stdlib mkdir build\test\stdlib
for %%i in (ring_writer ring_server ring_client ring_supervisor) do (
    echo building %%i ...
    bin\pikacc.exe test\stdlib\ring_common.pka test\stdlib\%%i.pka -o build\test\stdlib\%%i.exe
    if errorlevel 1 exit /b 1
)

build\test\stdlib\ring_supervisor.exe > build\test\stdlib\out.txt
for /f %%a in ('find /c /v "" ^< build\test\stdlib\out.txt') do set lines=%%a
if !lines! LSS 10 (
    echo Not enough lines (!lines!^)
    exit /b 1
)
echo Stdlib tests passed.
