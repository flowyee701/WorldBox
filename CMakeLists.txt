cmake_minimum_required(VERSION 3.20)
project(WorldBoxProto LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

find_package(raylib QUIET)

if (NOT raylib_FOUND)
    message(STATUS "raylib not found locally. Fetching from github...")

    FetchContent_Declare(
            raylib
            URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(raylib)
endif()

add_subdirectory(Project)

file(COPY "${CMAKE_SOURCE_DIR}/Project/assets"
        DESTINATION "${CMAKE_BINARY_DIR}")