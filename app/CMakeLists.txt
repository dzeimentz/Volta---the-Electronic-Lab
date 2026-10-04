# Qt-Anwendung (siehe docs/architecture.md, docs/adr/0001-qt6-stack.md)
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 Multimedia TextToSpeech Sensors Svg LinguistTools)
qt_standard_project_setup(REQUIRES 6.8)

set(VOLTA_APP_NAME "Volta 1 - Das Elektronik-Labor")
if(VOLTA_EDITION STREQUAL "volta2")
    set(VOLTA_APP_NAME "Volta 2 - Das Elektronik-Labor")
elseif(VOLTA_EDITION STREQUAL "volta3")
    set(VOLTA_APP_NAME "Volta 3 - Das Elektronik-Labor")
elseif(VOLTA_EDITION STREQUAL "zuse1")
    set(VOLTA_APP_NAME "Zuse 1 - Computertechnik fuer Einsteiger")
endif()

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
    VOLTA_VERSION="${PROJECT_VERSION}")

qt_add_qml_module(volta_app
    URI Volta
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

qt_add_translations(volta_app
    TS_FILES
        ${PROJECT_SOURCE_DIR}/i18n/volta_de.ts
        ${PROJECT_SOURCE_DIR}/i18n/volta_fr.ts
        ${PROJECT_SOURCE_DIR}/i18n/volta_en.ts
        ${PROJECT_SOURCE_DIR}/i18n/volta_lb.ts)

target_link_libraries(volta_app PRIVATE
    volta::core
    Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::TextToSpeech Qt6::Sensors Qt6::Svg)
