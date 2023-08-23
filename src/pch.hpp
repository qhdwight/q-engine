#include <entt/entity/registry.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include <vulkan/vulkan_raii.hpp>

#include <vma/vk_mem_alloc.h>

constexpr uint32_t eVK_API_VERSION_1_3 = VK_API_VERSION_1_3;
constexpr std::string_view eVK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
constexpr std::string_view eVK_EXT_DEBUG_UTILS_EXTENSION_NAME = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
constexpr std::string_view eVK_KHR_SWAPCHAIN_EXTENSION_NAME = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
constexpr std::string_view eVK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME = VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME;

// Must include after vulkan headers
#include <GLFW/glfw3.h>

constexpr int eGLFW_CLIENT_API = GLFW_CLIENT_API;
constexpr int eGLFW_NO_API = GLFW_NO_API;

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

constexpr std::string_view eIMGUI_VERSION = IMGUI_VERSION;

void ImGui_CheckVersion() {
    IMGUI_CHECKVERSION();
}
