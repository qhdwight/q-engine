cmake_minimum_required(VERSION 3.16)
project(game_engine)
set(CMAKE_CXX_STANDARD 23)

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
# ENTT
add_subdirectory(entt)
include_directories(entt/src)

file(GLOB SOURCE_FILES CONFIGURE_DEPEND S "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
message("${CMAKE_CXX_COMPILE_FEATURES}")
target_link_libraries(${PROJECT_NAME} glfw glm glslang glslang-default-resource-limits SPIRV)
target_compile_definitions(${PROJECT_NAME} PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX VK_USE_PLATFORM_WIN32_KHR)
endif ()
