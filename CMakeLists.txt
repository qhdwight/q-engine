cmake_minimum_required(VERSION 3.16)
project(game_engine)
set(CMAKE_CXX_STANDARD 20)

# GLSLANG
add_subdirectory(glslang)
include_directories(glslang)
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
# ENTT
add_subdirectory(entt)
include_directories(entt/src)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} glfw glm glslang glslang-default-resource-limits SPIRV)
target_compile_definitions(${PROJECT_NAME} PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX VK_USE_PLATFORM_WIN32_KHR)
elseif (APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC VK_USE_PLATFORM_MACOS_MVK)
endif ()
