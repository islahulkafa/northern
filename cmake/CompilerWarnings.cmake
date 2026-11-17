add_library(northern_warnings INTERFACE)

if(MSVC)
    target_compile_options(northern_warnings INTERFACE /W4 /WX /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(northern_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Werror
    )
endif()
