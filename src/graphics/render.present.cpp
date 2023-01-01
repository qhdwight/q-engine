#include "render.hpp"

Window::Window(vk::raii::Instance const& instance, std::string_view window_name, vk::Extent2D const& extent)
        : window_name{window_name} {
    glfwInit();
    std::cout << std::format("[GLFW] {} initialized\n", glfwGetVersionString());
    glfwSetErrorCallback([](int error, const char* msg) {
        std::cerr << std::format("[GLFW] Error {}: {}\n", error, msg);
    });
    if (!glfwVulkanSupported()) {
        throw std::runtime_error("[GLFW] Vulkan is not supported");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window_raw = glfwCreateWindow(static_cast<int>(extent.width), static_cast<int>(extent.height), window_name.data(), nullptr, nullptr);
    window_handle = {window_raw, [](GLFWwindow* p) {
        glfwDestroyWindow(p);
        glfwTerminate();
    }};

    VkSurfaceKHR raw_surface;
    if (glfwCreateWindowSurface(static_cast<VkInstance>(*instance), window_raw, nullptr, &raw_surface) != VK_SUCCESS)
        throw std::runtime_error("[GLFW] Failed to create window surface");

    surface = {instance, raw_surface};
}

vk::Extent2D Window::extent() const {
    int width, height;
    glfwGetFramebufferSize(window_handle.get(), &width, &height);
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

vk::SurfaceFormatKHR pick_surface_format(std::vector<vk::SurfaceFormatKHR> const& formats) {
    GAME_ASSERT(!formats.empty());

    // request several formats, the first found will be used
    auto it = std::ranges::find(formats, vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear});
    return it == formats.end() ? formats.front() : *it;
}

vk::PresentModeKHR pick_present_mode(std::vector<vk::PresentModeKHR> const& present_modes) {
    GAME_ASSERT(!present_modes.empty());

    auto it = std::ranges::find(present_modes, vk::PresentModeKHR::eMailbox);
    return it == present_modes.end() ? present_modes.front() : *it;
}

vk::raii::SwapchainKHR make_swapchain(VulkanContext& vk, vk::SwapchainKHR const& from_swapchain = {}) {
    GAME_ASSERT(*vk.device);
    GAME_ASSERT(*vk.window.surface);
    GAME_ASSERT(*vk.physical_device);

    vk::SurfaceKHR const& surface = *vk.window.surface;

    vk::SurfaceFormatKHR surface_format = pick_surface_format(vk.physical_device.getSurfaceFormatsKHR(surface));
    vk::SurfaceCapabilitiesKHR surface_capabilities = vk.physical_device.getSurfaceCapabilitiesKHR(surface);
    vk::PresentModeKHR present_mode = pick_present_mode(vk.physical_device.getSurfacePresentModesKHR(surface));
    vk::SwapchainCreateInfoKHR create_info(
            {},
            surface,
            surface_capabilities.minImageCount,
            surface_format.format,
            surface_format.colorSpace,
            vk.window.extent(),
            1,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive,
            {},
            surface_capabilities.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            present_mode,
            true,
            from_swapchain
    );
    std::cout << std::format("[Vulkan] Creating swapchain with format: {} {}\n",
                             vk::to_string(surface_format.format), vk::to_string(surface_format.colorSpace));
    if (vk.graphics_family_index != vk.present_family_index) {
        std::array<uint32_t, 2> queue_family_indices{vk.graphics_family_index, vk.present_family_index};
        // If the graphics and present queues are from different queue families, we either have to explicitly
        // transfer ownership of images between the queues, or we have to create the swapchain with imageSharingMode
        // as vk::SharingMode::eConcurrent
        create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount = queue_family_indices.size();
        create_info.pQueueFamilyIndices = queue_family_indices.data();
    }
    return {vk.device, create_info};
}

Swapchain::Swapchain(VulkanContext& vk, Swapchain&& from) : swapchain{make_swapchain(vk, *from.swapchain)} {
    from.swapchain = nullptr;
//    swapchain->getImages()
//    images = swapChain->getImages();
//
//    imageViews.reserve(images.size());
//    vk::ImageViewCreateInfo imageViewCreateInfo({}, {}, vk::ImageViewType::e2D, colorFormat, {},
//                                                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
//    for (auto image: images) {
//        imageViewCreateInfo.image = static_cast<vk::Image>( image );
//        imageViews.emplace_back(device, imageViewCreateInfo);
//    }
}

void create_swapchain(VulkanContext& vk) {

    Swapchain current = std::move(vk.swapchain);
    vk.swapchain = {vk, std::move(current)};

    VmaAllocationCreateInfo depth_alloc_info = {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    vk::ImageCreateInfo depth_image_info{
            {},
            vk::ImageType::e2D,
            vk::Format::eD32Sfloat,
            vk::Extent3D{vk.window.extent(), 1},
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::SharingMode::eExclusive,
            {},
            vk::ImageLayout::eUndefined
    };

    vk.depth_image = {vk.allocator, depth_alloc_info, depth_image_info};

//    auto [graphicsFamilyIdx, presentFamilyIdx] = vk::raii::su::findGraphicsAndPresentQueueFamilyIndex(*vk.physDev, *vk.surfData->surface);
//    vk.swapChainData.reset();
//    vk.swapChainData = vk::raii::su::SwapChainData(
//            *vk.physDev,
//            *vk.device,
//            *vk.surfData->surface,
//            vk.surfData->extent,
//            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
//            {},
//            graphicsFamilyIdx,
//            presentFamilyIdx
//    );
//    vk.depthBufferData.reset();
//    vk.depthBufferData = vk::raii::su::DepthBufferData(*vk.physDev, *vk.device, vk::raii::su::pickDepthFormat(*vk.physDev), vk.surfData->extent);
//    vk.renderPass.reset();
//    vk.renderPass = vk::raii::su::makeRenderPass(
//            *vk.device,
//            vk::su::pickSurfaceFormat(vk.physDev->getSurfaceFormatsKHR(**vk.surfData->surface)).format,
//            vk.depthBufferData->format
//    );
//    vk.framebufs = vk::raii::su::makeFramebuffers(
//            *vk.device,
//            *vk.renderPass,
//            vk.swapChainData->imageViews,
//            &*vk.depthBufferData->imageView,
//            vk.surfData->extent
//    );
}
