# Toolchain for building v8GitView with the MSYS2 mingw64 environment.
#
# Set MSYS2_ROOT to the MSYS2 installation directory, for example:
#   set MSYS2_ROOT=C:\msys64
#   cmake --preset mingw64-release
#
# The default matches a standard MSYS2 installation.

if(DEFINED ENV{MSYS2_ROOT} AND NOT "$ENV{MSYS2_ROOT}" STREQUAL "")
    set(MSYS2_ROOT "$ENV{MSYS2_ROOT}" CACHE PATH "MSYS2 installation root")
else()
    set(MSYS2_ROOT "C:/msys64" CACHE PATH "MSYS2 installation root")
endif()

set(MSYS2_MINGW64 "${MSYS2_ROOT}/mingw64")

if(NOT EXISTS "${MSYS2_MINGW64}/bin/g++.exe")
    message(FATAL_ERROR
        "MSYS2 mingw64 toolchain not found at '${MSYS2_MINGW64}'.\n"
        "Set the MSYS2_ROOT environment variable to your MSYS2 directory.")
endif()

set(CMAKE_CXX_COMPILER "${MSYS2_MINGW64}/bin/g++.exe" CACHE FILEPATH "")
set(CMAKE_RC_COMPILER "${MSYS2_MINGW64}/bin/windres.exe" CACHE FILEPATH "")

# Qt 6 and libgit2 live in the mingw64 prefix.
list(PREPEND CMAKE_PREFIX_PATH "${MSYS2_MINGW64}")
