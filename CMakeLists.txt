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
# without a display server.
add_library(hash_tools STATIC
    src/tools/toolregistrymodel.cpp
    src/tools/algorithmsmodel.cpp
    src/tools/hashfilemodel.cpp
    src/tools/hashcontroller.cpp
    src/tools/textdigesttool.cpp
    src/tools/codectool.cpp
    src/tools/jsontool.cpp
    src/tools/timestamptool.cpp
    src/tools/uuidtool.cpp
    src/tools/radixtool.cpp
    src/tools/colortool.cpp
    src/tools/passwordtool.cpp
)
target_include_directories(hash_tools PUBLIC src)
target_link_libraries(hash_tools PUBLIC hash_core Qt6::Core)

add_executable(hash src/cli/main.cpp)
target_link_libraries(hash PRIVATE hash_core)

qt_add_executable(hash-gui WIN32
    src/app/main.cpp
    src/app/clipservice.cpp
)
target_link_libraries(hash-gui PRIVATE hash_tools Qt6::Quick Qt6::QuickControls2)

# The QML files are compiled into the binary (qmlcachegen), so the shipped
# application carries no loose .qml files. Explicit aliases keep the module
# layout independent of the source directory structure.
set(HASH_GUI_QML_FILES
    Main
    Theme
    Toast
    PageHeader
    HomePage
    FileHashPage
    TextHashPage
    CodecPage
    JsonPage
    TimestampPage
    UuidPage
    RadixPage
    ColorPage
    PasswordPage
)
foreach(qml_file IN LISTS HASH_GUI_QML_FILES)
    set_source_files_properties(src/app/${qml_file}.qml PROPERTIES
        QT_RESOURCE_ALIAS "${qml_file}.qml")
endforeach()
set_source_files_properties(src/app/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(src/app/Toast.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(hash-gui
    URI Hash
    VERSION 1.0
    QML_FILES
        src/app/Main.qml
        src/app/Theme.qml
        src/app/Toast.qml
        src/app/PageHeader.qml
        src/app/HomePage.qml
        src/app/FileHashPage.qml
        src/app/TextHashPage.qml
        src/app/CodecPage.qml
        src/app/JsonPage.qml
        src/app/TimestampPage.qml
        src/app/UuidPage.qml
        src/app/RadixPage.qml
        src/app/ColorPage.qml
        src/app/PasswordPage.qml
)

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
