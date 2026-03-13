enable_testing()

add_executable(worldbox_tests
    basic_test.cpp
)

target_link_libraries(worldbox_tests PRIVATE
    GTest::gtest_main
    worldbox_core
)

include(GoogleTest)
gtest_discover_tests(worldbox_tests)
