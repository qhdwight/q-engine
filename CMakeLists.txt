cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

project(game LANGUAGES CXX)

### CMake ###
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
if (CMAKE_VERSION VERSION_LESS "3.27.0")
    set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
else ()
    set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "aa1f7df0-828a-4fcd-9afc-2dc80491aca7")
endif ()
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/experimental:module)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_EXTENSIONS OFF)
    add_compile_options(
            -Wno-read-modules-implicitly
            -Wno-experimental-header-units
            -fmodule-file=${CMAKE_BINARY_DIR}/glfw3.pcm
            -fmodule-file=${CMAKE_BINARY_DIR}/vulkan.pcm
            -fmodule-file=${CMAKE_BINARY_DIR}/vk_mem_alloc.pcm
    )
    add_link_options(-fuse-ld=lld)
endif ()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### libc++ ###
set(LIBCXX_BUILD /opt/libcxx)
message(STATUS ${LIBCXX_BUILD})
include(FetchContent)
FetchContent_Declare(
        std
        SOURCE_DIR ${LIBCXX_BUILD}/modules/c++/v1
)
FetchContent_MakeAvailable(std)

add_compile_options(
        -fprebuilt-module-path=${CMAKE_BINARY_DIR}/_deps/std-build/CMakeFiles/std.dir/
        -nostdinc++
        -isystem ${LIBCXX_BUILD}/include/c++/v1
)
add_link_options(
        -nostdlib++
        -L${LIBCXX_BUILD}/lib
        -Wl,-rpath,${LIBCXX_BUILD}/lib
)
link_libraries(std c++)

### Dependencies ###
add_subdirectory(pkg build/pkg EXCLUDE_FROM_ALL SYSTEM)
include_directories(pkg/rapidjson/include)
find_package(Vulkan REQUIRED)

### Modules ###
file(GLOB_RECURSE MODULE_SOURCE_FILES CONFIGURE_DEPENDS src/*.ixx)
add_library(GameModules)
target_sources(GameModules
        PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${MODULE_SOURCE_FILES}
)
target_include_directories(GameModules PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(GameModules PUBLIC
    EnTT::EnTT
    VulkanMemoryAllocator VulkanHppModule Vulkan::Vulkan
    glfw
    RapidJSON
    IMGUI
)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cxx)
add_executable(Game ${SOURCE_FILES})
target_link_libraries(Game PUBLIC GameModules)

add_compile_definitions(GLFW_INCLUDE_VULKAN)
if (MSVC)
    target_compile_options(Game PRIVATE /W3 /WX)
else ()
    target_compile_options(Game PRIVATE -Wall -Wextra -Wpedantic)
endif ()
