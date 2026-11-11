cmake_minimum_required(VERSION 3.24)

project(v8GitView
    VERSION 0.1.0
    DESCRIPTION "Fast Git history viewer for large 1C:Enterprise repositories"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)
find_package(libgit2 CONFIG REQUIRED)

if (TARGET libgit2::libgit2package)
    set(V8GITVIEW_LIBGIT2 libgit2::libgit2package)
elseif (TARGET libgit2::libgit2)
    set(V8GITVIEW_LIBGIT2 libgit2::libgit2)
else ()
    message(FATAL_ERROR "libgit2 CMake target not found")
endif ()

# MSYS2's libgit2 package records absolute paths from its build machine for
# transitive dependencies (e.g. D:/M/msys64/mingw64/lib/libssl.dll.a). Those
# are only needed for static linking; with a shared libgit2 the import library
# is self-contained and the DLL dependencies are resolved at runtime.
get_target_property(V8GITVIEW_LIBGIT2_LINKS ${V8GITVIEW_LIBGIT2}
                    INTERFACE_LINK_LIBRARIES)
if (V8GITVIEW_LIBGIT2_LINKS MATCHES "D:/M/msys64")
    set_property(TARGET ${V8GITVIEW_LIBGIT2} PROPERTY
                 INTERFACE_LINK_LIBRARIES "")
endif ()

qt_standard_project_setup()

qt_add_executable(v8GitView WIN32
    src/main.cpp

    src/app/MainWindow.h
    src/app/MainWindow.cpp

    src/git/CommitInfo.h
    src/git/GitRepository.h
    src/git/GitRepository.cpp

    src/history/GraphRow.h
    src/history/HistoryRow.h
    src/history/GraphLayout.h
    src/history/GraphLayout.cpp
    src/history/HistoryLoader.h
    src/history/HistoryLoader.cpp
    src/history/CommitModel.h
    src/history/CommitModel.cpp

    src/ui/Theme.h
    src/ui/Theme.cpp
    src/ui/CommitView.h
    src/ui/CommitView.cpp
    src/ui/CommitDelegate.h
    src/ui/CommitDelegate.cpp
)

target_include_directories(v8GitView PRIVATE src)

target_link_libraries(v8GitView PRIVATE
    Qt6::Widgets
    ${V8GITVIEW_LIBGIT2}
)

if (WIN32)
    target_link_libraries(v8GitView PRIVATE dwmapi)
endif ()

target_compile_definitions(v8GitView PRIVATE
    V8GITVIEW_VERSION="${PROJECT_VERSION}"
)

if (MSVC)
    target_compile_options(v8GitView PRIVATE /W4 /permissive-)
else ()
    target_compile_options(v8GitView PRIVATE -Wall -Wextra -Wpedantic)
endif ()
