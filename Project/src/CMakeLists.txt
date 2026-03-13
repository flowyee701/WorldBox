# Core gameplay library
add_library(worldbox_core
        world.cpp
        human_behavior.cpp
        civilian_behavior.cpp
        warrior_behavior.cpp
        bandit_behavior.cpp
        captain_behavior.cpp
        Animal.cpp
        Plant.cpp
        Meteorite.cpp
        meteorite_methods.cpp
)

# Public headers
target_include_directories(worldbox_core PUBLIC
    ${CMAKE_SOURCE_DIR}/Project/include
)

# External dependencies
target_link_libraries(worldbox_core PUBLIC raylib)
