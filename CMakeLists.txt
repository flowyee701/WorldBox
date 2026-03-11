cmake_minimum_required(VERSION 3.20)
project(WorldBoxProto)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
include(FetchContent)

# Пытаемся найти установленный raylib
find_package(raylib QUIET)

if (NOT raylib_FOUND)
    message(STATUS "raylib not found locally. Fetching from github via Git...")

    FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG        5.0 # Или "master" для самой свежей версии
    )

    FetchContent_MakeAvailable(raylib)
endif()

# Добавляем папку Project, как в твоей структуре [cite: 2, 3]
add_subdirectory(Project)