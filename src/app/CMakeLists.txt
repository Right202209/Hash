# The hash-gui executable also backs the "Hash" QML module, so this
# CMakeLists lives beside the QML files: qt_add_qml_module maps the module's
# types from the files relative to this directory, which is the layout the
# tooling documents.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

qt_add_executable(hash-gui WIN32
    main.cpp
    clipservice.cpp
    clipservice.h
)
target_link_libraries(hash-gui PRIVATE hash_tools Qt6::Quick Qt6::QuickControls2)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(Toast.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# The QML files are compiled into the binary (qmlcachegen), so the shipped
# application carries no loose .qml files.
qt_add_qml_module(hash-gui
    URI Hash
    VERSION 1.0
    QML_FILES
        Main.qml
        Theme.qml
        Toast.qml
        PageHeader.qml
        HomePage.qml
        FileHashPage.qml
        TextHashPage.qml
        CodecPage.qml
        JsonPage.qml
        TimestampPage.qml
        UuidPage.qml
        RadixPage.qml
        ColorPage.qml
        PasswordPage.qml
)
