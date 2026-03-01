cmake_minimum_required(VERSION 3.20)
project(WorldBoxProto LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib CONFIG REQUIRED)

add_subdirectory(Project)

# Копируем папку 'assets' в директорию, где лежит скомпилированный бинарник
file(COPY "${CMAKE_SOURCE_DIR}/Project/assets"
        DESTINATION "${CMAKE_BINARY_DIR}")
