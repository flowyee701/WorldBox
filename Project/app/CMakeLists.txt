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