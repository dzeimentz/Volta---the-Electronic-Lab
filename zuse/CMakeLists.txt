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
    QT_ANDROID_PACKAGE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/packaging/android-zuse" # keine Berechtigungen
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
        qml/GateShape.qml
        qml/DiscoverStage.qml
        qml/StoryPanel.qml
        qml/ToolboxPopup.qml
        qml/NumberStrip.qml
        qml/NumbersPopup.qml
        qml/PinPad.qml
        qml/PanelPort.qml
        qml/TruthPanel.qml
        qml/ZButton.qml
)
# Programmsymbol (Quelle resources/icons/zuse1.svg, Rasterbilder mit tools/make_icons.py)
if(WIN32)
    add_library(zuse_app_rc OBJECT resources/zuse1.rc) # ohne Include-Pfade: windres scheitert an Leerzeichen
    set_target_properties(zuse_app_rc PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    target_sources(zuse_app PRIVATE $<TARGET_OBJECTS:zuse_app_rc>)
endif()
qt_add_resources(zuse_app "zuse_icons" PREFIX "/zuse" BASE resources/icons FILES
    resources/icons/zuse1_64.png resources/icons/zuse1_256.png)
# Maskottchen Zetti (tools/make_zetti.py)
file(GLOB zuse_zetti RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS resources/zetti/*.png)
qt_add_resources(zuse_app "zuse_zetti" PREFIX "/zuse" BASE resources FILES ${zuse_zetti})
qt_add_resources(zuse_app "zuse_content" PREFIX "/zuse" BASE content FILES content/stufen.json)
target_link_libraries(zuse_app PRIVATE zuse_model Qt6::Quick Qt6::QuickControls2)

if(VOLTA_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(zuse_workbench_tests tests/test_workbench.cpp)
    target_link_libraries(zuse_workbench_tests PRIVATE zuse_model Qt6::Test)
    set_target_properties(zuse_workbench_tests PROPERTIES WIN32_EXECUTABLE OFF)
    # Inhaltspfad als erzeugter Header (CMake verwirft Definitionen mit '#' im Wert, vgl. app/CMakeLists.txt)
    file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/zuse_paths.hpp"
        CONTENT "#pragma once\n#define ZUSE_CONTENT_DIR \"@CMAKE_CURRENT_SOURCE_DIR@/content\"\n" @ONLY)
    target_include_directories(zuse_workbench_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    add_test(NAME zuse_workbench_tests COMMAND zuse_workbench_tests)
    if(WIN32)
        get_target_property(_qmake Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qtbin "${_qmake}" DIRECTORY)
        set_tests_properties(zuse_workbench_tests PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qtbin}")
    endif()
endif()
