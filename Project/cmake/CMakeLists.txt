cmake_minimum_required(VERSION 3.20)
project(WorldBoxProto)

set(CMAKE_CXX_STANDARD 20)

add_executable(WorldBoxProto ../app/main.cpp
        ../include/npc/npc.h
        ../include/environment/settlement.h
        ../include/environment/world.h
        ../src/world.cpp
        ../include/npc/human_behavior.h
        ../src/human_behavior.cpp
        ../include/npc/civilian_behavior.h
        ../include/npc/warrior_behavior.h
        ../include/npc/bandit_behavior.h
        ../src/civilian_behavior.cpp
        ../src/warrior_behavior.cpp
        ../src/bandit_behavior.cpp)

find_package(raylib CONFIG REQUIRED)
target_link_libraries(WorldBoxProto raylib)