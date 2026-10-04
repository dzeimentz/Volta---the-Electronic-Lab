cmake_minimum_required(VERSION 3.24)

project(Volta
    VERSION 0.1.0
    DESCRIPTION "Volta - Das Elektronik-Labor"
    HOMEPAGE_URL "https://dc-software-engineering.com"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Edition (Band): volta1 | volta2 | volta3 | zuse1
set(VOLTA_EDITION "volta1" CACHE STRING "Band der Volta-Reihe")
set_property(CACHE VOLTA_EDITION PROPERTY STRINGS volta1 volta2 volta3 zuse1)

option(VOLTA_BUILD_APP   "Qt-Anwendung bauen (benoetigt Qt 6.8+)" ON)
option(VOLTA_BUILD_TESTS "Tests bauen" ON)

enable_testing()

add_subdirectory(core)

if(VOLTA_BUILD_APP)
    add_subdirectory(app)
endif()
