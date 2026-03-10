cmake_minimum_required(VERSION 3.20)
project(WorldBoxProto)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# try system raylib first (macOS / brew)
find_package(raylib QUIET)

# fallback for Windows / fresh machines
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

find_package(raylib CONFIG REQUIRED)
target_link_libraries(WorldBoxProto raylib)