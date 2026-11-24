add_library(northern_core STATIC
    core/concurrency/task_queue.hpp
    core/concurrency/task_queue.cpp
    core/concurrency/thread_pool.hpp
    core/concurrency/thread_pool.cpp
    core/error/error.hpp
    core/error/error.cpp
    core/error/result.hpp
    core/logging/log_level.hpp
    core/logging/log_message.hpp
    core/logging/log_sink.hpp
    core/logging/log_sink.cpp
    core/logging/logger.hpp
    core/logging/logger.cpp
    core/math/geometry.hpp
    core/math/geometry.cpp
    core/math/color.hpp
    core/math/color.cpp
    core/math/math_utils.hpp
    core/math/transform2d.hpp
    core/math/transform2d.cpp
    core/memory/handle.hpp
    core/memory/handle_pool.hpp
    core/memory/native_handle.hpp
    core/memory/arena.hpp
    core/memory/arena.cpp
    core/signal/connection.hpp
    core/signal/trackable.hpp
    core/signal/trackable.cpp
    core/signal/signal.hpp
    core/signal/signal.cpp
    core/animation/easing.hpp
    core/animation/spring.hpp
    core/animation/property_animation.hpp
    core/animation/animation_manager.hpp
    core/animation/animation_manager.cpp
)
target_include_directories(northern_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)
target_compile_features(northern_core PUBLIC cxx_std_20)
target_link_libraries(northern_core PRIVATE northern_warnings)

add_library(northern_platform STATIC
    platform/key_codes.hpp
    platform/mouse_types.hpp
    platform/events.hpp
    platform/display.hpp
    platform/window_desc.hpp
    platform/native_handle.hpp
    platform/window.hpp
    platform/window.cpp
    platform/file_dialog.hpp
    platform/application.hpp
    platform/application.cpp
    platform/clipboard.hpp
    platform/clipboard.cpp
    platform/win32/win32_display.cpp
    platform/win32/win32_window.hpp
    platform/win32/win32_window.cpp
    platform/win32/win32_key_map.hpp
    platform/win32/win32_key_map.cpp
    platform/win32/win32_file_dialog.cpp
)
target_include_directories(northern_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
)
target_compile_features(northern_platform PUBLIC cxx_std_20)
target_link_libraries(northern_platform
    PUBLIC
        northern_core
    PRIVATE
        northern_warnings
        user32
        gdi32
        shell32
        ole32
        dwmapi
)

add_library(northern_graphics STATIC
    graphics/types.hpp
    graphics/gpu_handle.hpp
    graphics/buffer.hpp
    graphics/texture.hpp
    graphics/pipeline.hpp
    graphics/command_buffer.hpp
    graphics/swapchain.hpp
    graphics/device.hpp
    graphics/device.cpp
    graphics/mock/mock_device.hpp
    graphics/mock/mock_device.cpp
    graphics/vulkan/vk_common.hpp
    graphics/vulkan/vk_staging_pool.hpp
    graphics/vulkan/vk_staging_pool.cpp
    graphics/vulkan/vk_mem_alloc.hpp
    graphics/vulkan/vk_mem_alloc.cpp
    graphics/vulkan/vk_buffer.hpp
    graphics/vulkan/vk_buffer.cpp
    graphics/vulkan/vk_texture.hpp
    graphics/vulkan/vk_texture.cpp
    graphics/vulkan/vk_pipeline.hpp
    graphics/vulkan/vk_pipeline.cpp
    graphics/vulkan/vk_command_buffer.hpp
    graphics/vulkan/vk_command_buffer.cpp
    graphics/vulkan/vk_swapchain.hpp
    graphics/vulkan/vk_swapchain.cpp
    graphics/vulkan/vk_device.hpp
    graphics/vulkan/vk_device.cpp
)
target_include_directories(northern_graphics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics
)
target_compile_features(northern_graphics PUBLIC cxx_std_20)
target_link_libraries(northern_graphics
    PUBLIC
        northern_core
    PRIVATE
        northern_warnings
        Vulkan::Vulkan
)

add_library(northern_text STATIC
    text/types.hpp
    text/font.hpp
    text/font_impl.hpp
    text/font.cpp
    text/font_manager.hpp
    text/font_manager.cpp
    text/text_shaper.hpp
    text/text_shaper.cpp
    text/text_layout.hpp
    text/text_layout.cpp
    text/glyph_atlas.hpp
    text/glyph_atlas.cpp
    text/vector_rasterizer.hpp
    text/vector_rasterizer.cpp
)
target_include_directories(northern_text PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/text
)
target_compile_features(northern_text PUBLIC cxx_std_20)
target_link_libraries(northern_text
    PUBLIC
        northern_core
        northern_graphics
    PRIVATE
        northern_warnings
        harfbuzz::harfbuzz
        Freetype::Freetype
)

add_library(northern_layout STATIC
    layout/types.hpp
    layout/length.hpp
    layout/style.hpp
    layout/layout_node.hpp
    layout/layout_node.cpp
    layout/flex_solver.hpp
    layout/flex_solver.cpp
)
target_include_directories(northern_layout PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/layout
)
target_compile_features(northern_layout PUBLIC cxx_std_20)
target_link_libraries(northern_layout
    PUBLIC
        northern_core
    PRIVATE
        northern_warnings
)

add_library(northern_renderer STATIC
    renderer/vertex_2d.hpp
    renderer/draw_command.hpp
    renderer/paint_context.hpp
    renderer/paint_context.cpp
    renderer/dynamic_ring_buffer.hpp
    renderer/dynamic_ring_buffer.cpp
    renderer/batch_renderer.hpp
    renderer/batch_renderer.cpp
    renderer/svg/nanosvg.h
    renderer/svg/svg_document.hpp
    renderer/svg/svg_document.cpp
)
target_include_directories(northern_renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer
    ${CMAKE_SOURCE_DIR}
)
target_compile_features(northern_renderer PUBLIC cxx_std_20)
target_link_libraries(northern_renderer
    PUBLIC
        northern_core
        northern_graphics
        northern_text
    PRIVATE
        northern_warnings
)

add_library(northern_widgets STATIC
    # Base
    widgets/base/widget_events.hpp
    widgets/base/theme.hpp
    widgets/base/overlay.hpp
    widgets/base/widget.hpp
    widgets/base/widget.cpp
    widgets/base/widget_context.hpp
    widgets/base/widget_context.cpp

    # Controls
    widgets/controls/label.hpp
    widgets/controls/label.cpp
    widgets/controls/button.hpp
    widgets/controls/button.cpp
    widgets/controls/checkbox.hpp
    widgets/controls/checkbox.cpp
    widgets/controls/radio_button.hpp
    widgets/controls/radio_button.cpp
    widgets/controls/radio_group.hpp
    widgets/controls/radio_group.cpp
    widgets/controls/switch.hpp
    widgets/controls/switch.cpp
    widgets/controls/slider.hpp
    widgets/controls/slider.cpp
    widgets/controls/text_input.hpp
    widgets/controls/text_input.cpp
    widgets/controls/progress_bar.hpp
    widgets/controls/progress_bar.cpp
    widgets/controls/divider.hpp
    widgets/controls/divider.cpp
    widgets/controls/icon.hpp
    widgets/controls/icon.cpp
    widgets/controls/icons.hpp
    widgets/controls/icons.cpp
    widgets/controls/tooltip.hpp
    widgets/controls/tooltip.cpp

    # Containers
    widgets/containers/container.hpp
    widgets/containers/container.cpp
    widgets/containers/scroll_view.hpp
    widgets/containers/scroll_view.cpp
    widgets/containers/split_view.hpp
    widgets/containers/split_view.cpp
    widgets/containers/tab_view.hpp
    widgets/containers/tab_view.cpp
    widgets/containers/accordion.hpp
    widgets/containers/accordion.cpp
    widgets/containers/dropdown.hpp
    widgets/containers/dropdown.cpp
    widgets/containers/context_menu.hpp
    widgets/containers/context_menu.cpp
    widgets/containers/menu_bar.hpp
    widgets/containers/menu_bar.cpp

    # Views
    widgets/views/item_model.hpp
    widgets/views/item_model.cpp
    widgets/views/item_selection_model.hpp
    widgets/views/item_selection_model.cpp
    widgets/views/item_delegate.hpp
    widgets/views/item_delegate.cpp
    widgets/views/header_view.hpp
    widgets/views/header_view.cpp
    widgets/views/list_view.hpp
    widgets/views/list_view.cpp
    widgets/views/table_view.hpp
    widgets/views/table_view.cpp
    widgets/views/tree_view.hpp
    widgets/views/tree_view.cpp

    # Window Shell
    widgets/dialogs/title_bar.hpp
    widgets/dialogs/title_bar.cpp
)
target_include_directories(northern_widgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/base
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/controls
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/containers
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/views
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/dialogs
)
target_compile_features(northern_widgets PUBLIC cxx_std_20)
target_link_libraries(northern_widgets
    PUBLIC
        northern_core
        northern_platform
        northern_layout
        northern_renderer
        northern_text
    PRIVATE
        northern_warnings
)
