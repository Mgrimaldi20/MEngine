@echo off
setlocal

if "%~1"=="" (
    echo Usage: %~nx0 [debug|release]
    exit /b 1
)

set CONFIG=%~1

if "%CONFIG%"=="debug" (
    set PRESET=windows-debug
) else if "%CONFIG%"=="release" (
    set PRESET=windows-release
) else (
    echo Invalid configuration
    exit /b 1
)

cd /d %~dp0

cmake --preset %PRESET%

cmake --build out/build/%PRESET% --target install

endlocal
