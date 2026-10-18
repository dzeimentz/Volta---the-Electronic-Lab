cmake_minimum_required(VERSION 3.24)

project(Volta
    VERSION 0.9.0 # Testphase vor 1.0; Android-versionCode folgt daraus (app/CMakeLists.txt)
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
add_subdirectory(logic) # Logiksimulation der Zuse-Reihe (Qt-frei)

if(VOLTA_BUILD_APP)
    add_subdirectory(app)
    # Zuse-Prototyp (Phase Z0): eigene App; auf Android und iPadOS nur auf Wunsch (eigenes Paket)
    if(ANDROID OR IOS)
        option(VOLTA_BUILD_ZUSE "Zuse-Prototyp bauen" OFF)
    else()
        option(VOLTA_BUILD_ZUSE "Zuse-Prototyp bauen" ON)
    endif()
    if(VOLTA_BUILD_ZUSE)
        add_subdirectory(zuse)
    endif()
endif()
