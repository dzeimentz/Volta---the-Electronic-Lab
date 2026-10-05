# Qt-Anwendung (siehe docs/architecture.md, docs/adr/0001-qt6-stack.md)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Multimedia TextToSpeech Sensors Svg LinguistTools)
qt_standard_project_setup(REQUIRES 6.8)

set(VOLTA_APP_NAME "Volta 1 - Das Elektronik-Labor")
if(VOLTA_EDITION STREQUAL "volta2")
    set(VOLTA_APP_NAME "Volta 2 - Das Elektronik-Labor")
elseif(VOLTA_EDITION STREQUAL "volta3")
    set(VOLTA_APP_NAME "Volta 3 - Das Elektronik-Labor")
elseif(VOLTA_EDITION STREQUAL "zuse1")
    set(VOLTA_APP_NAME "Zuse 1 - Computertechnik fuer Einsteiger")
endif()

# Schaltungsmodell und Dienste: ohne Oberflaeche testbar (Qt Core fuer JSON)
add_library(volta_model STATIC
    src/model/anchor.cpp
    src/model/board_edit.cpp
    src/model/netlist_builder.cpp
    src/model/success.cpp
    src/services/content_repository.cpp
    src/sim/simulation_bridge.cpp
    src/model/lab_session.cpp
    src/model/commands/board_command.cpp
)
target_include_directories(volta_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
find_package(Threads REQUIRED)
target_link_libraries(volta_model PUBLIC volta::core Qt6::Core Qt6::Gui Threads::Threads)

qt_add_executable(volta_app
    src/main.cpp
)
set_target_properties(volta_app PROPERTIES
    OUTPUT_NAME "${VOLTA_EDITION}"
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    QT_ANDROID_PACKAGE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/packaging/android"
    QT_ANDROID_MIN_SDK_VERSION 26)

target_compile_definitions(volta_app PRIVATE
    VOLTA_EDITION="${VOLTA_EDITION}"
    VOLTA_APP_NAME="${VOLTA_APP_NAME}"
    VOLTA_VERSION="${PROJECT_VERSION}"
    VOLTA_CONTENT_DIR="${PROJECT_SOURCE_DIR}/content")

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(volta_app
    URI Volta
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/Lab.qml
        qml/ContactField.qml
        qml/VoltageBars.qml
        qml/PartItem.qml
        qml/WireItem.qml
    SOURCES
        src/lab_controller.hpp
        src/lab_controller.cpp
)

# Uebersetzungen: qt_add_lrelease statt qt_add_translations, weil Letzteres (Qt 6.11) an Leerzeichen im
# Quellpfad scheitert ("VOLTA Elektronik-Labor"). lupdate bei Bedarf von Hand: lupdate app -ts i18n/volta_de.ts
qt_add_lrelease(volta_app
    TS_FILES
        ${PROJECT_SOURCE_DIR}/i18n/volta_de.ts
        ${PROJECT_SOURCE_DIR}/i18n/volta_fr.ts
        ${PROJECT_SOURCE_DIR}/i18n/volta_en.ts
        ${PROJECT_SOURCE_DIR}/i18n/volta_lb.ts
    QM_FILES_OUTPUT_VARIABLE volta_qm_files)
qt_add_resources(volta_app "translations" PREFIX "/i18n" BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${volta_qm_files})

target_link_libraries(volta_app PRIVATE
    volta_model
    Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::TextToSpeech Qt6::Sensors Qt6::Svg)

if(VOLTA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
