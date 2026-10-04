find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(Catch2)
endif()

add_executable(volta_core_tests
    test_circuit.cpp
)
target_link_libraries(volta_core_tests PRIVATE volta::core Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(volta_core_tests)
