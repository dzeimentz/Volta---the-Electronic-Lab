# Catch2 stellt core/tests bereit (gleiche Version, wird nur einmal geholt)
add_executable(volta_logic_tests
    test_gates.cpp
    test_chips.cpp
)
target_link_libraries(volta_logic_tests PRIVATE volta::logic Catch2::Catch2WithMain)

find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_GetProperties(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()
include(Catch)
catch_discover_tests(volta_logic_tests)
