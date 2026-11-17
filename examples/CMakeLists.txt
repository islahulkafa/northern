add_executable(example_windowing
    windowing/main.cpp
)

target_link_libraries(example_windowing
    PRIVATE
        northern_widgets
        northern_layout
        northern_renderer
        northern_text
        northern_graphics
        northern_platform
        northern_core
        northern_warnings
)

target_include_directories(example_windowing
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(example_windowing PRIVATE cxx_std_20)
