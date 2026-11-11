@echo off
setlocal EnableExtensions

rem Run v8GitView with the Qt/libgit2 DLLs from MSYS2.
rem
rem Usage:
rem   scripts\run_mingw64.bat [repository] [MSYS2_ROOT] [debug|release]
rem
rem Example:
rem   scripts\run_mingw64.bat C:\src\my-repo C:\work\cpp\msys64 release

set "REPO=%~1"
set "MSYS2_ROOT_ARG=%~2"
set "CONFIG=%~3"

if not "%MSYS2_ROOT_ARG%"=="" set "MSYS2_ROOT=%MSYS2_ROOT_ARG%"
if "%MSYS2_ROOT%"=="" (
    if exist "C:\msys64\mingw64\bin\g++.exe" set "MSYS2_ROOT=C:\msys64"
)
if "%MSYS2_ROOT%"=="" (
    echo MSYS2_ROOT is not set.
    echo Pass the MSYS2 directory as the second argument.
    exit /b 1
)

if "%CONFIG%"=="" set "CONFIG=release"
set "PATH=%MSYS2_ROOT%\mingw64\bin;%PATH%"

set "EXE=%~dp0..\build\mingw64-%CONFIG%\v8GitView.exe"
if not exist "%EXE%" (
    echo Executable not found: %EXE%
    echo Build it first with scripts\build_mingw64.bat.
    exit /b 1
)

if "%REPO%"=="" (
    start "" "%EXE%"
) else (
    start "" "%EXE%" "%REPO%"
)
endlocal
