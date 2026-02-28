cmake_minimum_required(VERSION 3.20)
project(WorldBoxProto)

set(CMAKE_CXX_STANDARD 20)

add_executable(WorldBoxProto ../app/main.cpp
        ../include/npc.h
        ../include/settlement.h
        ../include/world.h
        ../src/world.cpp
        ../include/human_behavior.h
        ../src/human_behavior.cpp
        ../include/civilian_behavior.h
        ../include/warrior_behavior.h
        ../include/bandit_behavior.h
        ../src/civilian_behavior.cpp
        ../src/warrior_behavior.cpp
        ../src/bandit_behavior.cpp)

find_package(raylib CONFIG REQUIRED)
target_link_libraries(WorldBoxProto raylib)