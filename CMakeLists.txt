cmake_minimum_required(VERSION 3.20)
project(WorldBoxProto)

set(CMAKE_CXX_STANDARD 20)

add_executable(WorldBoxProto main.cpp
        npc.h
        settlement.h
        world.h
        world.cpp
        human_behavior.h
        human_behavior.cpp
        civilian_behavior.h
        warrior_behavior.h
        bandit_behavior.h
        civilian_behavior.cpp
        warrior_behavior.cpp
        bandit_behavior.cpp)

find_package(raylib CONFIG REQUIRED)
target_link_libraries(WorldBoxProto raylib)