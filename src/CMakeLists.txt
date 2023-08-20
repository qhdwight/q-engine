cmake_minimum_required(VERSION 3.25)
project(game)

### Dependencies ###
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(ENV{VULKAN_SDK} "C:\\VulkanSDK\\1.3.250.1")
find_package(Vulkan REQUIRED)
add_subdirectory(../pkg ../build/pkg EXCLUDE_FROM_ALL SYSTEM)

set(CMAKE_CXX_STANDARD 23)

#set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
#if (MSVC)
#    set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
#endif ()

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
add_executable(game ${SOURCE_FILES})
target_include_directories(game PRIVATE ${PROJECT_SOURCE_DIR})
target_precompile_headers(game PRIVATE game_pch.hpp graphics/render.hpp)
target_link_libraries(game PUBLIC glfw glslang glslang-default-resource-limits SPIRV Edyn VulkanMemoryAllocator Vulkan::Vulkan RapidJSON IMGUI)

target_compile_definitions(game PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=0)
if (WIN32)
    target_compile_definitions(game PUBLIC NOMINMAX VK_USE_PLATFORM_WIN32_KHR)
elseif (APPLE)
    target_compile_definitions(game PUBLIC VK_USE_PLATFORM_MACOS_MVK)
elseif (UNIX)
    target_compile_definitions(game PUBLIC VK_USE_PLATFORM_XCB_KHR)
endif ()

if (MSVC)
    target_compile_options(game PRIVATE /W3 /WX)
else ()
    target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic)
endif ()
