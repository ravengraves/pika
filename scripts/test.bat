@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

if not exist build\test mkdir build\test
for %%i in (test\*.pka) do (
    set "base=%%~ni"
    echo building build\test\!base! ...
    bin\pikacc.exe %%i -o build\test\!base!.exe
    if errorlevel 1 exit /b 1
    build\test\!base!.exe
    if errorlevel 1 exit /b 1
)
echo All language tests passed.
