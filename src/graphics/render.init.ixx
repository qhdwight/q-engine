module;

#include <pch.hpp>

export module render:init;

import :debug;

import game;
import logging;

import vulkan;

import std;

constexpr std::string_view APP_NAME = "Game Engine";
constexpr uint32_t APP_VERSION = 1;
constexpr std::string_view ENGINE_NAME = "QEngine";
constexpr uint32_t ENGINE_VERSION = 1;

using InstanceExtensions = std::vector<char const*>;
using DeviceExtensions = std::vector<char const*>;
using Layers = std::vector<char const*>;

export struct QueueFamilyIndices {
    uint32_t graphicsFamilyIndex{};
    uint32_t presentFamilyIndex{};
};

[[nodiscard]] InstanceExtensions getInstanceExtensions(vk::raii::Context const& context) {
    InstanceExtensions desiredExtensions{VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for (uint32_t i = 0; i < glfwExtensionCount; ++i)
            desiredExtensions.emplace_back(glfwExtensions[i]);
    }
    if constexpr (IS_DEBUG) {
        desiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    log("[Vulkan] Desired instance extensions:");
    for (std::string_view extension: desiredExtensions) {
        log("\t{}", extension);
    }

    std::vector<vk::ExtensionProperties> availableExtensions = context.enumerateInstanceExtensionProperties();
    std::vector<std::string_view> sortedDesiredExtensions(desiredExtensions.size());
    std::ranges::partial_sort_copy(desiredExtensions, sortedDesiredExtensions);
    std::vector<std::string_view> sortedAvailableExtensions(availableExtensions.size());
    std::ranges::partial_sort_copy(availableExtensions | std::views::transform([](const auto& e) { return std::string_view{e.extensionName}; }),
                                   sortedAvailableExtensions);
    log("[Vulkan] Available instance extensions:");
    for (std::string_view extension: sortedAvailableExtensions) {
        log("\t{}", extension);
    }

    std::vector<std::string_view> missingExtensions;
    std::ranges::set_difference(sortedDesiredExtensions, sortedAvailableExtensions, std::back_inserter(missingExtensions));

    if (!missingExtensions.empty()) {
        std::ostringstream out;
        out << "[Vulkan] Missing extensions:\n";
        for (std::string_view extension: missingExtensions) {
            out << std::format("\t{}", extension);
        }
        throw std::runtime_error(out.str());
    }

    return desiredExtensions;
}

[[nodiscard]] DeviceExtensions getDeviceExtensions() {
    DeviceExtensions extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};
    if constexpr (OS == OS::macOS) {
        extensions.push_back("VK_KHR_portability_subset");
    }
    // TODO: check if physical device supports extensions
    return extensions;
}

export vk::raii::Instance makeInstance(vk::raii::Context const& context) {
    vk::ApplicationInfo appInfo{APP_NAME.data(), APP_VERSION, ENGINE_NAME.data(), ENGINE_VERSION, VK_API_VERSION_1_3};

    InstanceExtensions extensions = getInstanceExtensions(context);
    Layers layers;
    if constexpr (IS_DEBUG && OS == OS::Linux) {
//        layers.emplace_back("VK_LAYER_KHRONOS_validation");
    }

    log("[Vulkan] Available instance layers:");
    for (auto const& layer: vk::enumerateInstanceLayerProperties()) {
        log("\t{}", std::string_view{layer.layerName});
    }

    vk::InstanceCreateInfo createInfo{vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR, &appInfo, layers, extensions};
    return {context, createInfo};
}

export vk::raii::DebugUtilsMessengerEXT makeDebugUtilsMessenger(vk::raii::Instance const& instance) {
    check(*instance);

    using
    enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using
    enum vk::DebugUtilsMessageTypeFlagBitsEXT;
//    return instance->createDebugUtilsMessengerEXTUnique(
//            vk::DebugUtilsMessengerCreateInfoEXT{{}, eWarning | eError, eGeneral | ePerformance | eValidation, &debugUtilsMessengerCallback},
//            nullptr,
//
//            );
    return {
            instance,
            vk::DebugUtilsMessengerCreateInfoEXT{{}, eWarning | eError, eGeneral | ePerformance | eValidation, &debugUtilsMessengerCallback},
    };
}

struct PhysicalDeviceComparator {
    [[nodiscard]] static uint32_t score(vk::raii::PhysicalDevice const& device) {
        uint32_t score = 0;
        auto properties = device.getProperties();
        switch (properties.deviceType) {
            case vk::PhysicalDeviceType::eDiscreteGpu:
                score += 400;
                break;
            case vk::PhysicalDeviceType::eIntegratedGpu:
                score += 300;
                break;
            case vk::PhysicalDeviceType::eVirtualGpu:
                score += 200;
                break;
            case vk::PhysicalDeviceType::eCpu:
                score += 100;
                break;
            default:
                break;
        }
        return score;
    }

    bool operator()(vk::raii::PhysicalDevice const& d1, vk::raii::PhysicalDevice const& d2) const {
        return score(d1) > score(d2);
    }
};

vk::raii::PhysicalDevice makePhysicalDevice(vk::raii::Instance const& instance) {
    check(*instance);

    auto physicalDevices = vk::raii::PhysicalDevices{instance};
    if (physicalDevices.empty()) {
        throw std::runtime_error("No physical devices found");
    }

    vk::raii::PhysicalDevice physicalDevice = std::move(*std::ranges::max_element(physicalDevices, PhysicalDeviceComparator{}));

    vk::PhysicalDeviceProperties const& properties = physicalDevice.getProperties();
    log("[Vulkan] Chose physical device {}", properties.deviceName.data());
    uint32_t apiVersion = properties.apiVersion;
    log("[Vulkan] {}.{}.{} device API version",
        VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion));
    log("[Vulkan] Available physical device extensions:");
    for (auto const& extension: physicalDevice.enumerateDeviceExtensionProperties()) {
        log("\t{}", std::string_view{extension.extensionName});
    }
    return physicalDevice;
}

QueueFamilyIndices findQueueFamilyIndices(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::SurfaceKHR const& surface) {
    check(*physicalDevice);
    check(*surface);

    using QueueFamilyIndex = uint32_t;
    using QueueFamily = std::tuple<QueueFamilyIndex, vk::QueueFamilyProperties>;

    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

//    auto queueFamilies = std::views::zip(std::views::iota(QueueFamilyIndex{0}), queueFamilyProperties);
//
//    auto graphicsQueues = queueFamilies | std::views::filter([](QueueFamily const& queue) {
//                              auto const& [index, properties] = queue;
//                              return static_cast<bool>(properties.queueFlags & vk::QueueFlagBits::eGraphics);
//                          }) |
//                          std::views::keys | std::ranges::to<std::vector<QueueFamilyIndex>>();
//    if (graphicsQueues.empty()) throw std::runtime_error("No graphics queues found");
//
//    auto presentationQueues = queueFamilies | std::views::filter([&](QueueFamily const& queue) {
//                                  auto const& [index, properties] = queue;
//                                  return physicalDevice.getSurfaceSupportKHR(index, *surface);
//                              }) |
//                              std::views::keys | std::ranges::to<std::vector<QueueFamilyIndex>>();
//    if (presentationQueues.empty()) throw std::runtime_error("No presentation queues found");
//
//    std::vector<QueueFamilyIndex> combined;
//    std::ranges::set_intersection(graphicsQueues, presentationQueues, std::back_inserter(combined));
//    if (!combined.empty()) return {combined.front(), combined.front()};
//
//    return {graphicsQueues.front(), presentationQueues.front()};
    return {};
}

vk::raii::Device makeLogicalDevice(vk::raii::PhysicalDevice const& physicalDevice, uint32_t graphicsFamilyIndex) {
    check(*physicalDevice);

    DeviceExtensions extensions = getDeviceExtensions();
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo({}, graphicsFamilyIndex, 1, &queuePriority);
    vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{true};
    vk::DeviceCreateInfo createInfo{{}, queueCreateInfo, {}, extensions, {}, &dynamicRenderingFeatures};
    return {physicalDevice, createInfo};
}

vk::raii::DescriptorPool makeDescriptorPool(vk::raii::Device const& device, std::vector<vk::DescriptorPoolSize> const& poolSizes) {
    check(*device);
    check(!poolSizes.empty());

    uint32_t maxSets = std::accumulate(poolSizes.begin(), poolSizes.end(), 0,
                                       [](uint32_t sum, vk::DescriptorPoolSize const& dps) { return sum + dps.descriptorCount; });
    check(maxSets > 0);
    vk::DescriptorPoolCreateInfo createInfo{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, maxSets, poolSizes};
    return {device, createInfo};
}

template<typename Func>
void oneTimeSubmit(vk::raii::CommandBuffer const& commandBuffer, vk::raii::Queue const& queue, Func const& func) {
    commandBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    func(commandBuffer);
    commandBuffer.end();
    queue.submit(vk::SubmitInfo{nullptr, nullptr, *commandBuffer}, nullptr);
    queue.waitIdle();
}

export void setupGlfw() {
    glfwInit();
    std::cout << std::format("[GLFW] {} initialized\n", glfwGetVersionString());
    glfwSetErrorCallback([](int error, const char* msg) {
        std::cerr << std::format("[GLFW] Error {}: {}\n", error, msg);
    });
    if (!glfwVulkanSupported()) throw std::runtime_error("[GLFW] Vulkan is not supported");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}
