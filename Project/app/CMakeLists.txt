# App target
add_executable(WorldBoxProto
    main.cpp
)

# Include paths
target_include_directories(WorldBoxProto
    PRIVATE
    ${CMAKE_SOURCE_DIR}/Project/include
)

# App dependencies
target_link_libraries(WorldBoxProto
    PRIVATE
    worldbox_core
)

# Copy runtime assets
add_custom_command(TARGET WorldBoxProto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    $<TARGET_FILE_DIR:WorldBoxProto>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/../assets
    $<TARGET_FILE_DIR:WorldBoxProto>/assets
)
