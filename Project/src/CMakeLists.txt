add_library(worldbox_core
        world.cpp
        human_behavior.cpp
        civilian_behavior.cpp
        warrior_behavior.cpp
        bandit_behavior.cpp
        captain_behavior.cpp
        Animal.cpp        # ДОБАВИТЬ ЭТО
        Plant.cpp # ДОБАВИТЬ ЭТО
)

target_include_directories(worldbox_core PUBLIC
        ${CMAKE_SOURCE_DIR}/Project/include
)

target_link_libraries(worldbox_core PUBLIC raylib)