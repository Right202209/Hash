cmake_minimum_required(VERSION 3.21)

project(Hash VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.3 REQUIRED COMPONENTS Core Widgets Test)
qt_standard_project_setup()

if(MSVC)
    add_compile_options(/utf-8 /W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra)
endif()

add_library(hash_core STATIC
    src/common/fspath.cpp
    src/common/pathkey.cpp
    src/common/quoting.cpp
    src/common/atomicfile.cpp
    src/common/compare.cpp
    src/core/digesters.cpp
    src/core/registry.cpp
    src/core/hasher.cpp
    src/core/hasher_windows.cpp
    src/core/hasher_posix.cpp
    src/core/batch.cpp
    src/cli/format.cpp
    src/cli/paths.cpp
    src/cli/options.cpp
)
target_include_directories(hash_core PUBLIC src)
target_link_libraries(hash_core PUBLIC Qt6::Core)

add_executable(hash src/cli/main.cpp)
target_link_libraries(hash PRIVATE hash_core)

add_executable(hash-gui WIN32
    src/app/main.cpp
    src/app/theme.cpp
    src/app/worker.cpp
    src/app/mainwindow.cpp
)
target_link_libraries(hash-gui PRIVATE hash_core Qt6::Widgets)

enable_testing()

add_executable(hash_tests
    tests/main.cpp
    tests/test_registry.h
    tests/test_hasher.h
    tests/test_batch.h
    tests/test_atomicfile.h
    tests/test_compare.h
    tests/test_format.h
    tests/test_paths.h
    tests/test_pathkey.h
    tests/test_progress.h
)
target_link_libraries(hash_tests PRIVATE hash_core Qt6::Test)
add_test(NAME hash_tests COMMAND hash_tests)
