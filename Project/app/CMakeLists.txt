add_executable(WorldBoxProto
        main.cpp
)

# Подключаем include директорию
target_include_directories(WorldBoxProto
        PRIVATE
        ${CMAKE_SOURCE_DIR}/Project/include
)

# Линкуем библиотеку с логикой
target_link_libraries(WorldBoxProto
        PRIVATE
        worldbox_core
)

add_custom_command(TARGET WorldBoxProto POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:WorldBoxProto>/assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/../assets
        $<TARGET_FILE_DIR:WorldBoxProto>/assets
)