find_package(harfbuzz CONFIG REQUIRED)
find_package(Freetype REQUIRED)
find_package(Vulkan REQUIRED)

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
endif()
