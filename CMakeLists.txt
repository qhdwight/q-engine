cmake_minimum_required(VERSION 3.16)
project(game_engine)
set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    add_compile_options(/Wv:18)
endif ()

# Vulkan
if (WIN32)
    set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.2.198.1")
endif ()
find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})
# GLFW
add_subdirectory(glfw)
# GLM
add_subdirectory(glm)
include_directories(glm)
# GLSLANG
add_subdirectory(glslang)
include_directories(glslang)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(game_engine ${SOURCE_FILES})
target_link_libraries(game_engine glfw glm glslang glslang-default-resource-limits SPIRV ${PHYSX_LIBRARIES})
target_compile_definitions(game_engine PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
if (WIN32)
    target_compile_definitions(game_engine PUBLIC NOMINMAX VK_USE_PLATFORM_WIN32_KHR)
endif ()
