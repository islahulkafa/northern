cmake_minimum_required(VERSION 3.25)
project(northern
    VERSION 0.1.0
    DESCRIPTION "GPU-accelerated desktop UI framework"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/CompilerWarnings.cmake)
include(CTest)
include(cmake/Dependencies.cmake)

add_subdirectory(src)
add_subdirectory(examples)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_target(sync_compile_commands ALL
        ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
        COMMENT "Syncing compile_commands.json to workspace root for language servers"
    )
endif()
