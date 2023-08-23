if (MSVC)
    add_compile_options(/w)
else ()
    add_compile_options(-w)
endif ()

set(GLFW_INSTALL OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)

set(ENABLE_CTEST OFF)

#set(SPIRV_REFLECT_EXAMPLES OFF)
#set(SPIRV_REFLECT_EXECUTABLE OFF)
#set(SPIRV_REFLECT_STATIC_LIB ON)

set(RAPIDJSON_BUILD_DOC OFF)
set(RAPIDJSON_BUILD_CXX17 ON)
set(RAPIDJSON_BUILD_TESTS OFF)
set(RAPIDJSON_BUILD_EXAMPLES OFF)

set(VULKAN_HPP_ENABLE_EXPERIMENTAL_CPP20_MODULES ON)

include(FetchContent)
FetchContent_Declare(
        entt
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/entt
        OVERRIDE_FIND_PACKAGE SYSTEM
)
FetchContent_Declare(
        glfw
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/glfw
        OVERRIDE_FIND_PACKAGE SYSTEM
)
FetchContent_Declare(
        glslang
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/glslang
        OVERRIDE_FIND_PACKAGE SYSTEM
)
FetchContent_Declare(
        vulkan-hpp
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/vulkan-hpp
        OVERRIDE_FIND_PACKAGE SYSTEM
)
FetchContent_Declare(
        vma
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/vma
        OVERRIDE_FIND_PACKAGE SYSTEM
)
FetchContent_Declare(
        rapidjson
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/rapidjson
        OVERRIDE_FIND_PACKAGE SYSTEM
)
FetchContent_MakeAvailable(entt glfw vulkan-hpp vma rapidjson)

add_library(IMGUI
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_demo.cpp
        imgui/backends/imgui_impl_vulkan.cpp
        imgui/backends/imgui_impl_glfw.cpp
)
target_include_directories(IMGUI PUBLIC imgui)
target_link_libraries(IMGUI PUBLIC glfw VulkanHppModule)
