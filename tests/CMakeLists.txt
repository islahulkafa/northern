include(GoogleTest)

set(UNIT_TEST_SOURCES
    unit/test_result.cpp
    unit/test_handle_pool.cpp
    unit/test_signal.cpp
    unit/test_geometry.cpp
    unit/test_color.cpp
    unit/test_platform.cpp
    unit/test_graphics.cpp
    unit/test_renderer.cpp
    unit/test_text.cpp
    unit/test_layout.cpp
    unit/test_widgets.cpp
)

add_executable(unit_tests ${UNIT_TEST_SOURCES})
target_link_libraries(unit_tests
    PRIVATE
        northern_core
        northern_platform
        northern_graphics
        northern_renderer
        northern_text
        northern_layout
        northern_widgets
        GTest::gtest
        GTest::gtest_main
)
target_include_directories(unit_tests PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}")

gtest_discover_tests(unit_tests
    PROPERTIES
        LABELS "unit"
        DISCOVERY_TIMEOUT 15
)
