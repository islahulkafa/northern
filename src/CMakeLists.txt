add_library(northern_core STATIC
    core/error/error.hpp
    core/error/error.cpp
    core/error/result.hpp
    core/math/geometry.hpp
    core/math/geometry.cpp
    core/math/color.hpp
    core/math/color.cpp
    core/memory/handle.hpp
    core/memory/handle_pool.hpp
    core/memory/native_handle.hpp
    core/signal/connection.hpp
    core/signal/trackable.hpp
    core/signal/trackable.cpp
    core/signal/signal.hpp
    core/signal/signal.cpp
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
    platform/window_desc.hpp
    platform/native_handle.hpp
    platform/window.hpp
    platform/window.cpp
    platform/application.hpp
    platform/application.cpp
    platform/clipboard.hpp
    platform/clipboard.cpp
    platform/win32/win32_window.hpp
    platform/win32/win32_window.cpp
    platform/win32/win32_key_map.hpp
    platform/win32/win32_key_map.cpp
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
    graphics/vulkan/vk_common.hpp
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
    widgets/widget_events.hpp
    widgets/widget.hpp
    widgets/widget.cpp
    widgets/widget_context.hpp
    widgets/widget_context.cpp
    widgets/container.hpp
    widgets/container.cpp
    widgets/label.hpp
    widgets/label.cpp
    widgets/button.hpp
    widgets/button.cpp
    widgets/text_input.hpp
    widgets/text_input.cpp
    widgets/scroll_view.hpp
    widgets/scroll_view.cpp
    widgets/checkbox.hpp
    widgets/checkbox.cpp
    widgets/slider.hpp
    widgets/slider.cpp
)
target_include_directories(northern_widgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
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

add_library(northern_richtext INTERFACE)
target_include_directories(northern_richtext INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/richtext
)
target_compile_features(northern_richtext INTERFACE cxx_std_20)
target_link_libraries(northern_richtext INTERFACE
    northern_widgets
    northern_text
    northern_renderer
    northern_core
)

add_library(northern_devtools INTERFACE)
target_include_directories(northern_devtools INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/devtools
)
target_compile_features(northern_devtools INTERFACE cxx_std_20)
target_link_libraries(northern_devtools INTERFACE
    northern_widgets
    northern_renderer
    northern_core
)
