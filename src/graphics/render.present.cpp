#include "render.hpp"

Window::Window(vk::raii::Instance const& instance, std::string_view window_name, vk::Extent2D const& extent)
    : window_name{window_name} {

    GLFWwindow* windowRaw = glfwCreateWindow(static_cast<int>(extent.width), static_cast<int>(extent.height), window_name.data(), nullptr, nullptr);
    windowHandle = {windowRaw, glfwDestroyWindow};

    VkSurfaceKHR rawSurface;
    if (glfwCreateWindowSurface(static_cast<VkInstance>(*instance), windowRaw, nullptr, &rawSurface) != VK_SUCCESS)
        throw std::runtime_error("[GLFW] Failed to create window surface");

    surface = {instance, rawSurface};
}

vk::Extent2D Window::extent() const {
    int width, height;
    glfwGetFramebufferSize(windowHandle.get(), &width, &height);
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

vk::SurfaceFormatKHR pick_surface_format(std::vector<vk::SurfaceFormatKHR> const& formats) {
    GAME_ASSERT(!formats.empty());

    // request several formats, the first found will be used
    auto it = std::ranges::find(formats, vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear});
    return it == formats.end() ? formats.front() : *it;
}

vk::PresentModeKHR pick_present_mode(std::vector<vk::PresentModeKHR> const& presentModes) {
    GAME_ASSERT(!presentModes.empty());

    auto it = std::ranges::find(presentModes, vk::PresentModeKHR::eMailbox);
    return it == presentModes.end() ? presentModes.front() : *it;
}


Swapchain::Swapchain(VulkanContext& vk, Swapchain&& from) {
    GAME_ASSERT(*vk.device);
    GAME_ASSERT(*vk.window.surface);
    GAME_ASSERT(*vk.physicalDevice);

    vk::SurfaceKHR const& surface = *vk.window.surface;

    format = pick_surface_format(vk.physicalDevice.getSurfaceFormatsKHR(surface));
    extent = vk.window.extent();
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = vk.physicalDevice.getSurfaceCapabilitiesKHR(surface);
    vk::PresentModeKHR presentMode = pick_present_mode(vk.physicalDevice.getSurfacePresentModesKHR(surface));
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
    if (vk.graphicsFamilyIndex != vk.presentFamilyIndex) {
        std::array<uint32_t, 2> queueFamilyIndices{vk.graphicsFamilyIndex, vk.presentFamilyIndex};
        // If the graphics and present queues are from different queue families, we either have to explicitly
        // transfer ownership of images between the queues, or we have to create the swapchain with imageSharingMode
        // as vk::SharingMode::eConcurrent
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    }
    swapchain = {vk.device, createInfo};

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
        views.emplace_back(vk.device, viewCreateInfo);
    }

    // Invalidate swapchain we are moving from
    from.swapchain = nullptr;
}

void createSwapchain(VulkanContext& vk) {
    Swapchain current = std::move(vk.swapchain);
    vk.swapchain = {vk, std::move(current)};
    vk.depthImage.emplace(vk);
}
