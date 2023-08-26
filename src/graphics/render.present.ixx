export module render:present;

import :init;

import common;
import logging;

import std;
import glfw;
import vulkan;
import vulkanc;

struct Window {

    Window() = default;

    Window(vk::raii::Instance const& instance, const char* windowName, vk::Extent2D const& extent)
        : windowName{windowName} {

        GLFWwindow* windowRaw = glfwCreateWindow(
                static_cast<int>(extent.width), static_cast<int>(extent.height),
                windowName,// Enforce null-termination
                nullptr, nullptr);
        windowHandle = {windowRaw, glfwDestroyWindow};

        VkSurfaceKHR rawSurface;
        if (auto result = static_cast<vk::Result>(glfwCreateWindowSurface(*instance, windowRaw, nullptr, &rawSurface)); result != vk::Result::eSuccess)
            throw std::runtime_error("[GLFW] Failed to create window surface: " + vk::to_string(result));

        surface = {instance, rawSurface};
    }

    Window(const Window&) = delete;

    Window& operator=(Window&) = delete;

    Window(Window&&) = default;

    Window& operator=(Window&&) = default;

    [[nodiscard]] vk::Extent2D extent() const {
        int width, height;
        glfwGetFramebufferSize(windowHandle.get(), &width, &height);
        return {static_cast<u32>(width), static_cast<u32>(height)};
    }

    std::string windowName;
    std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>> windowHandle;
    vk::raii::SurfaceKHR surface = nullptr;
};

vk::PresentModeKHR pickPresentMode(std::span<vk::PresentModeKHR> presentModes) {
    check(!presentModes.empty());

    auto it = std::ranges::find(presentModes, vk::PresentModeKHR::eMailbox);
    return it == presentModes.end() ? presentModes.front() : *it;
}

vk::SurfaceFormatKHR pickSurfaceFormat(std::span<vk::SurfaceFormatKHR> formats) {
    check(!formats.empty());

    // request several formats, the first found will be used
    auto it = std::ranges::find(formats, vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear});
    return it == formats.end() ? formats.front() : *it;
}

struct Swapchain {

    Swapchain() = default;

    Swapchain(vk::raii::Device const& device, vk::raii::PhysicalDevice const& physicalDevice, QueueFamilyIndices const& queueFamilyIndices,
              Window const& window, Swapchain&& from) {
        vk::SurfaceKHR const& surface = *window.surface;
        std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
        format = pickSurfaceFormat(formats);
        extent = window.extent();
        vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
        vk::PresentModeKHR presentMode = pickPresentMode(presentModes);
        vk::SwapchainCreateInfoKHR createInfo{
                {},
                surface,
                surfaceCapabilities.minImageCount,
                format.format,
                format.colorSpace,
                extent,
                1,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                vk::SharingMode::eExclusive,
                {},
                surfaceCapabilities.currentTransform,
                vk::CompositeAlphaFlagBitsKHR::eOpaque,
                presentMode,
                true,
                *from.swapchain,
        };
        log("[Vulkan] Creating swapchain with format: {} {}",
            vk::to_string(format.format), vk::to_string(format.colorSpace));
        if (queueFamilyIndices.graphicsFamilyIndex != queueFamilyIndices.presentFamilyIndex) {
            std::array array{queueFamilyIndices.graphicsFamilyIndex, queueFamilyIndices.presentFamilyIndex};
            // If the graphics and present queues are from different queue families, we either have to explicitly
            // transfer ownership of images between the queues, or we have to create the swapchain with imageSharingMode
            // as vk::SharingMode::eConcurrent
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = static_cast<u32>(array.size());
            createInfo.pQueueFamilyIndices = array.data();
        }
        swapchain = {device, createInfo};

        // Create views
        std::vector<vk::Image> images = swapchain.getImages();
        views.reserve(images.size());
        for (vk::Image const& image: images) {
            vk::ImageViewCreateInfo viewCreateInfo{
                    {},
                    image,
                    vk::ImageViewType::e2D,
                    format.format,
                    {},
                    {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
            };
            views.emplace_back(device, viewCreateInfo);
        }

        // Invalidate swapchain we are moving from
        from.swapchain = nullptr;
    }

    Swapchain(const Swapchain&) = delete;

    Swapchain& operator=(Swapchain&) = delete;

    Swapchain(Swapchain&&) = default;

    Swapchain& operator=(Swapchain&&) = default;

    vk::raii::SwapchainKHR swapchain = nullptr;
    vk::Extent2D extent;
    vk::SurfaceFormatKHR format;
    std::vector<vk::raii::ImageView> views;
};
