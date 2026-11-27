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
    unit/test_image.cpp
    unit/test_text.cpp
    unit/test_layout.cpp
    unit/test_widgets.cpp
    unit/test_media_widgets.cpp
    unit/test_button.cpp
    unit/test_label.cpp
    unit/test_checkbox.cpp
    unit/test_switch.cpp
    unit/test_radio_button.cpp
    unit/test_slider.cpp
    unit/test_progress_bar.cpp
    unit/test_text_input.cpp
    unit/test_text_area.cpp
    unit/test_animation.cpp
    unit/test_overlay.cpp
    unit/test_dropdown.cpp
    unit/test_focus_cursor_tooltip.cpp
    unit/test_context_menu.cpp
    unit/test_divider.cpp
    unit/test_split_view.cpp
    unit/test_tab_view.cpp
    unit/test_scroll_view.cpp
    unit/test_accordion.cpp
    unit/test_container.cpp
    unit/test_menu_bar.cpp
    unit/test_item_model.cpp
    unit/test_item_selection_model.cpp
    unit/test_header_view.cpp
    unit/test_list_view.cpp
    unit/test_table_view.cpp
    unit/test_tree_view.cpp
    unit/test_collection_views.cpp
    unit/test_svg.cpp
    unit/test_icon.cpp
    unit/test_title_bar.cpp
    unit/test_dialog.cpp
    unit/test_file_dialog.cpp
    unit/test_transform2d.cpp
    unit/test_arena.cpp
    unit/test_logging.cpp
    unit/test_task_queue.cpp
    unit/test_thread_pool.cpp
    unit/test_mock_device.cpp
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
