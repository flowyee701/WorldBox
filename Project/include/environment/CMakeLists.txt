add_library(environment_core INTERFACE)

target_include_directories(environment_core
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(environment_core
        INTERFACE
        npc_core
)