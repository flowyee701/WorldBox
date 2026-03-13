add_library(npc_core INTERFACE
        captain_behavior.h)

target_include_directories(npc_core
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)