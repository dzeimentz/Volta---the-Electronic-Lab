# Qt-Anwendung (siehe docs/architecture.md, docs/adr/0001-qt6-stack.md)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Multimedia TextToSpeech Sensors Svg LinguistTools)
qt_standard_project_setup(REQUIRES 6.8)

set(VOLTA_APP_NAME "Volta 1 - Das Elektronik-Labor")
set(VOLTA_APP_SHORT_NAME "Volta 1") # Name unter dem Symbol (Android, iPadOS)
if(VOLTA_EDITION STREQUAL "volta2")
    set(VOLTA_APP_NAME "Volta 2 - Das Elektronik-Labor")
    set(VOLTA_APP_SHORT_NAME "Volta 2")
elseif(VOLTA_EDITION STREQUAL "volta3")
    set(VOLTA_APP_NAME "Volta 3 - Das Elektronik-Labor")
    set(VOLTA_APP_SHORT_NAME "Volta 3")
elseif(VOLTA_EDITION STREQUAL "zuse1")
    set(VOLTA_APP_NAME "Zuse 1 - Computertechnik fuer Einsteiger")
    set(VOLTA_APP_SHORT_NAME "Zuse 1")
endif()
# App-ID (Android-Paketname, iOS-Bundle-ID); vor der ersten Veroeffentlichung festlegen, danach unveraenderlich
set(VOLTA_APP_ID "com.dcsoftwareengineering.${VOLTA_EDITION}" CACHE STRING "App-ID fuer Android und iOS")

# Schaltungsmodell und Dienste: ohne Oberflaeche testbar (Qt Core fuer JSON)
add_library(volta_model STATIC
    src/model/anchor.cpp
    src/model/board_edit.cpp
    src/model/netlist_builder.cpp
    src/model/success.cpp
    src/services/content_repository.cpp
    src/services/progress_store.cpp
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

# Inhaltspakete als Qt-Ressource (:/content): auf Android und iOS immer, weil es dort keinen Ordner neben der
# ausfuehrbaren Datei gibt; auf dem Desktop optional (-DVOLTA_EMBED_CONTENT=ON). Schemata und CLAUDE.md bleiben
# draussen. main.cpp nimmt zuerst content/ neben der ausfuehrbaren Datei, dann :/content, dann den Quellbaum.
if(ANDROID OR IOS)
    set(_volta_embed_default ON)
else()
    set(_volta_embed_default OFF)
endif()
option(VOLTA_EMBED_CONTENT "Inhaltspakete als Qt-Ressource in die App einbetten" ${_volta_embed_default})
file(GLOB_RECURSE VOLTA_CONTENT_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/content/*")
list(FILTER VOLTA_CONTENT_FILES EXCLUDE REGEX "/content/(schema/|CLAUDE\\.md$)")
function(volta_embed_content target)
    # relative Pfade: qt_add_resources mit BASE ausserhalb des Quellverzeichnisses
    set(_files "")
    foreach(_f IN LISTS VOLTA_CONTENT_FILES)
        file(RELATIVE_PATH _rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_f}")
        list(APPEND _files "${_rel}")
    endforeach()
    file(RELATIVE_PATH _base "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/content")
    qt_add_resources(${target} "volta_content" PREFIX "/content" BASE "${_base}" FILES ${_files})
endfunction()
if(VOLTA_EMBED_CONTENT)
    volta_embed_content(volta_app)
endif()
# App-Symbol (Quelle resources/icons/volta.svg, Rasterbilder mit tools/make_icons.py)
if(WIN32)
    # eigene Objektbibliothek ohne Include-Pfade: windres (MinGW) scheitert an Leerzeichen in Include-Pfaden
    add_library(volta_app_rc OBJECT resources/volta.rc)
    set_target_properties(volta_app_rc PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    target_sources(volta_app PRIVATE $<TARGET_OBJECTS:volta_app_rc>)
endif()

set_target_properties(volta_app PROPERTIES
    OUTPUT_NAME "${VOLTA_EDITION}"
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    QT_ANDROID_PACKAGE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/packaging/android"
    QT_ANDROID_MIN_SDK_VERSION 26)

# Android (docs/platforms.md): Manifest, Symbole und Startthema in packaging/android; Versionscode aus der
# Projektversion (0.1.0 -> 100, 1.2.3 -> 10203)
if(ANDROID)
    math(EXPR VOLTA_VERSION_CODE
         "${PROJECT_VERSION_MAJOR} * 10000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")
    set_target_properties(volta_app PROPERTIES
        QT_ANDROID_PACKAGE_NAME "${VOLTA_APP_ID}"
        QT_ANDROID_APP_NAME "${VOLTA_APP_SHORT_NAME}"
        QT_ANDROID_VERSION_NAME "${PROJECT_VERSION}"
        QT_ANDROID_VERSION_CODE "${VOLTA_VERSION_CODE}")
endif()

target_compile_definitions(volta_app PRIVATE
    VOLTA_EDITION="${VOLTA_EDITION}"
    VOLTA_APP_NAME="${VOLTA_APP_NAME}"
    VOLTA_VERSION="${PROJECT_VERSION}"
    VOLTA_CONTENT_DIR="${PROJECT_SOURCE_DIR}/content")

# Oberflaeche als statische QML-Bibliothek (URI Volta): von der App und von den Oberflaechentests genutzt
set_source_files_properties(qml/Theme.qml qml/Texts.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_library(volta_ui STATIC)
qt_add_qml_module(volta_ui
    URI Volta
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/Texts.qml
        qml/Lab.qml
        qml/ContactField.qml
        qml/VoltageBars.qml
        qml/PartItem.qml
        qml/PartBody.qml
        qml/WireItem.qml
        qml/WireEndHandle.qml
        qml/PartTray.qml
        qml/Toast.qml
        qml/KnobRing.qml
        qml/ControlDesk.qml
        qml/GroupLine.qml
        qml/ExperimentPanel.qml
        qml/InfoDialog.qml
        qml/Splash.qml
        qml/MeterPanel.qml
        qml/SpeakerRing.qml
        qml/SchematicSymbol.qml
        qml/TopBarActions.qml
        qml/Home.qml
    SOURCES
        src/lab_controller.hpp
        src/lab_controller.cpp
        src/language_service.hpp
        src/language_service.cpp
        src/app_settings.hpp
        src/app_settings.cpp
)
target_link_libraries(volta_ui PUBLIC volta_model Qt6::Quick Qt6::QuickControls2)
# Programmsymbol (Fenster, Info), Flaggen, Logo: in volta_ui, damit App und UI-Tests sie haben
qt_add_resources(volta_ui "icons" PREFIX "/icons" BASE resources/icons FILES
    resources/icons/volta_32.png resources/icons/volta_64.png resources/icons/volta_256.png)
# Runde Flaggen der Anzeigesprachen (Werkzeugleiste oben rechts)
qt_add_resources(volta_ui "flags" PREFIX "/flags" BASE resources/flags FILES
    resources/flags/de.svg resources/flags/fr.svg resources/flags/en.svg resources/flags/lb.svg)
# Firmenlogo fuer das Info-Fenster (optional; ohne Datei zeigt Info den Schriftzug)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/brand/dc_logo.png")
    qt_add_resources(volta_ui "brand" PREFIX "/brand" BASE resources/brand FILES resources/brand/dc_logo.png)
endif()

# Uebersetzungen: qt_add_lrelease statt qt_add_translations, weil Letzteres (Qt 6.11) an Leerzeichen im
# Quellpfad scheitert ("VOLTA Elektronik-Labor"). lupdate bei Bedarf von Hand: lupdate app -ts i18n/volta_de.ts
qt_add_lrelease(volta_ui
    TS_FILES
        ${PROJECT_SOURCE_DIR}/i18n/volta_de.ts
        ${PROJECT_SOURCE_DIR}/i18n/volta_fr.ts
        ${PROJECT_SOURCE_DIR}/i18n/volta_en.ts
        ${PROJECT_SOURCE_DIR}/i18n/volta_lb.ts
    QM_FILES_OUTPUT_VARIABLE volta_qm_files)
qt_add_resources(volta_ui "translations" PREFIX "/i18n" BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${volta_qm_files})

target_link_libraries(volta_app PRIVATE
    volta_ui volta_uiplugin
    Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::TextToSpeech Qt6::Sensors Qt6::Svg)

if(VOLTA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
