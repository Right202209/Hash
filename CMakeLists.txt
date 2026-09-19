cmake_minimum_required(VERSION 3.21)

project(Hash VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Quick QuickControls2 Test)
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

# hash_tools hosts the toolbox backends: Qt object models and tool controllers
# that the QML layer binds to. It stays QtCore-only so the tests exercise it
# without a display server. Headers with Q_OBJECT are listed so AUTOMOC
# processes them regardless of include-path resolution.
add_library(hash_tools STATIC
    src/tools/batchworker.h
    src/tools/toolregistrymodel.cpp
    src/tools/toolregistrymodel.h
    src/tools/algorithmsmodel.cpp
    src/tools/algorithmsmodel.h
    src/tools/hashfilemodel.cpp
    src/tools/hashfilemodel.h
    src/tools/hashcontroller.cpp
    src/tools/hashcontroller.h
    src/tools/textdigesttool.cpp
    src/tools/textdigesttool.h
    src/tools/codectool.cpp
    src/tools/codectool.h
    src/tools/jsontool.cpp
    src/tools/jsontool.h
    src/tools/timestamptool.cpp
    src/tools/timestamptool.h
    src/tools/uuidtool.cpp
    src/tools/uuidtool.h
    src/tools/radixtool.cpp
    src/tools/radixtool.h
    src/tools/colortool.cpp
    src/tools/colortool.h
    src/tools/passwordtool.cpp
    src/tools/passwordtool.h
)
target_include_directories(hash_tools PUBLIC src)
target_link_libraries(hash_tools PUBLIC hash_core Qt6::Core)

add_executable(hash src/cli/main.cpp)
target_link_libraries(hash PRIVATE hash_core)

# The GUI target and its QML module are defined in src/app, next to the QML
# files; see src/app/CMakeLists.txt.
add_subdirectory(src/app)

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
    tests/test_textdigest.h
    tests/test_codecs.h
    tests/test_jsontool.h
    tests/test_timestamptool.h
    tests/test_generatortools.h
    tests/test_colortool.h
    tests/test_hashcontroller.h
)
target_link_libraries(hash_tests PRIVATE hash_tools Qt6::Test)
add_test(NAME hash_tests COMMAND hash_tests)
