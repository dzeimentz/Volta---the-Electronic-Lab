# Logiksimulation der Zuse-Reihe - Qt-frei wie der Volta-Kern (docs/Konzept_Zuse.md, ADR-0010)
add_library(volta_logic STATIC
    src/chip.cpp
    src/netlist.cpp
    src/logic_sim.cpp
)
add_library(volta::logic ALIAS volta_logic)

target_include_directories(volta_logic PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

if(MSVC)
    target_compile_options(volta_logic PRIVATE /W4 /permissive-)
else()
    target_compile_options(volta_logic PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if(VOLTA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
