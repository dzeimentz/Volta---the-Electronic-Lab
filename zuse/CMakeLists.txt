# Zuse-Prototyp Z0: Werkbank (docs/Konzept_Zuse.md, ADR-0010). Eigene App neben Volta, Logik aus logic/.
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.8)

# Werkbank-Modell ohne Oberflaeche (testbar)
add_library(zuse_model STATIC
    src/chip_json.cpp
    src/workbench_controller.cpp
)
target_include_directories(zuse_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(zuse_model PUBLIC volta::logic Qt6::Core Qt6::Gui)
set_target_properties(zuse_model PROPERTIES AUTOMOC ON)

qt_add_executable(zuse_app src/main.cpp)
set_target_properties(zuse_app PROPERTIES
    OUTPUT_NAME "zuse1"
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    QT_ANDROID_PACKAGE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/packaging/android" # keine Berechtigungen
    QT_ANDROID_PACKAGE_NAME "com.dcsoftwareengineering.zuse1"
    QT_ANDROID_VERSION_NAME "0.1.0"
    QT_ANDROID_VERSION_CODE 100
    QT_ANDROID_MIN_SDK_VERSION 26
    QT_ANDROID_TARGET_SDK_VERSION 36)

set_source_files_properties(qml/ZTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(zuse_app
    URI Zuse
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/ZTheme.qml
        qml/Workbench.qml
        qml/GateBody.qml
        qml/PinPad.qml
        qml/TruthPanel.qml
        qml/ZButton.qml
)
qt_add_resources(zuse_app "zuse_content" PREFIX "/zuse" BASE content FILES content/stufen.json)
target_link_libraries(zuse_app PRIVATE zuse_model Qt6::Quick Qt6::QuickControls2)

if(VOLTA_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(zuse_workbench_tests tests/test_workbench.cpp)
    target_link_libraries(zuse_workbench_tests PRIVATE zuse_model Qt6::Test)
    set_target_properties(zuse_workbench_tests PROPERTIES WIN32_EXECUTABLE OFF)
    target_compile_definitions(zuse_workbench_tests PRIVATE ZUSE_CONTENT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/content")
    add_test(NAME zuse_workbench_tests COMMAND zuse_workbench_tests)
    if(WIN32)
        get_target_property(_qmake Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qtbin "${_qmake}" DIRECTORY)
        set_tests_properties(zuse_workbench_tests PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qtbin}")
    endif()
endif()
