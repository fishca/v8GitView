@echo off
setlocal EnableExtensions
cd /d "%~dp0.."

rem Build v8GitView with the MSYS2 mingw64 toolchain.
rem
rem Usage:
rem   scripts\build_mingw64.bat [MSYS2_ROOT] [debug|release]
rem
rem Example:
rem   scripts\build_mingw64.bat C:\work\cpp\msys64 release
rem
rem CMake and Ninja must be available in PATH.

set "MSYS2_ROOT_ARG=%~1"
set "CONFIG=%~2"

if not "%MSYS2_ROOT_ARG%"=="" set "MSYS2_ROOT=%MSYS2_ROOT_ARG%"
if "%MSYS2_ROOT%"=="" (
    if exist "C:\msys64\mingw64\bin\g++.exe" set "MSYS2_ROOT=C:\msys64"
)
if "%MSYS2_ROOT%"=="" (
    echo MSYS2_ROOT is not set.
    echo Pass the MSYS2 directory as the first argument, for example:
    echo   scripts\build_mingw64.bat C:\msys64 release
    exit /b 1
)

if "%CONFIG%"=="" set "CONFIG=release"
if /i not "%CONFIG%"=="debug" if /i not "%CONFIG%"=="release" (
    echo Unknown configuration '%CONFIG%'. Use debug or release.
    exit /b 1
)

set "PATH=%MSYS2_ROOT%\mingw64\bin;%PATH%"

echo Configuring v8GitView (%CONFIG%) with MSYS2 at %MSYS2_ROOT%...
cmake --preset mingw64-%CONFIG%
if errorlevel 1 exit /b 1

echo Building...
cmake --build --preset mingw64-%CONFIG%
if errorlevel 1 exit /b 1

echo.
echo Built: build\mingw64-%CONFIG%\v8GitView.exe
endlocal
