add_library(terrain_core
    terrain.cpp
)

target_include_directories(terrain_core PUBLIC
    ${CMAKE_SOURCE_DIR}/Project/include
)
target_link_libraries(terrain_core PUBLIC raylib)
