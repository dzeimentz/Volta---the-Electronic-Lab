# Simulationskern - Qt-frei (siehe docs/adr/0002-qt-freier-kern.md)
add_library(volta_core STATIC
    src/version.cpp
    src/mna_system.cpp
    src/dense_lu.cpp
    src/sim_event.cpp
    src/component_model.cpp
    src/circuit.cpp
    src/simulator.cpp
    src/components/resistor.cpp
    src/components/battery.cpp
    src/components/diode.cpp
    src/components/capacitor.cpp
    src/components/wire.cpp
)
add_library(volta::core ALIAS volta_core)

target_include_directories(volta_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_definitions(volta_core PUBLIC
    VOLTA_CORE_VERSION="${PROJECT_VERSION}")

if(MSVC)
    target_compile_options(volta_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(volta_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if(VOLTA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
