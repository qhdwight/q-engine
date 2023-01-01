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


Swapchain::Swapchain(VulkanContext& vk, Swapchain&& from) {
    GAME_ASSERT(*vk.device);
    GAME_ASSERT(*vk.window.surface);
    GAME_ASSERT(*vk.physical_device);

    vk::SurfaceKHR const& surface = *vk.window.surface;

    format = pick_surface_format(vk.physical_device.getSurfaceFormatsKHR(surface));
    vk::SurfaceCapabilitiesKHR surface_capabilities = vk.physical_device.getSurfaceCapabilitiesKHR(surface);
    vk::PresentModeKHR present_mode = pick_present_mode(vk.physical_device.getSurfacePresentModesKHR(surface));
    vk::SwapchainCreateInfoKHR create_info(
            {},
            surface,
            surface_capabilities.minImageCount,
            format.format, format.colorSpace,
            vk.window.extent(),
            1, // image array layers
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive,
            {}, // queue family indices
            surface_capabilities.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            present_mode,
            true, // clipped
            *from.swapchain
    );
    std::cout << std::format("[Vulkan] Creating swapchain with format: {} {}\n", vk::to_string(format.format), vk::to_string(format.colorSpace));
    if (vk.graphics_family_index != vk.present_family_index) {
        std::array<uint32_t, 2> queue_family_indices{vk.graphics_family_index, vk.present_family_index};
        // If the graphics and present queues are from different queue families, we either have to explicitly
        // transfer ownership of images between the queues, or we have to create the swapchain with imageSharingMode
        // as vk::SharingMode::eConcurrent
        create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount = queue_family_indices.size();
        create_info.pQueueFamilyIndices = queue_family_indices.data();
    }
    swapchain = {vk.device, create_info};

    // Create views
    std::vector<vk::Image> images = swapchain.getImages();
    views.reserve(images.size());
    vk::ImageViewCreateInfo view_create_info{{}, {}, vk::ImageViewType::e2D, format.format, {},
                                             {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
    for (vk::Image const& image: images) {
        view_create_info.image = static_cast<vk::Image>(image);
        views.emplace_back(vk.device, view_create_info);
    }

    // Invalidate swapchain we are moving from
    from.swapchain = nullptr;
}

vk::raii::RenderPass make_render_pass(VulkanContext& vk) {
    GAME_ASSERT(*vk.device);
    GAME_ASSERT(*vk.swapchain.swapchain);

    std::array<vk::AttachmentDescription, 2> attachment_descriptions{
            vk::AttachmentDescription{
                    {},
                    vk.swapchain.format.format,
                    vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare,
                    vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::ePresentSrcKHR
            },
            vk::AttachmentDescription{
                    {},
                    DEPTH_FORMAT,
                    vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eDontCare,
                    vk::AttachmentLoadOp::eDontCare,
                    vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal
            }
    };
    vk::AttachmentReference color_attachment{0, vk::ImageLayout::eColorAttachmentOptimal};
    vk::AttachmentReference depth_attachment{1, vk::ImageLayout::eDepthStencilAttachmentOptimal};
    vk::SubpassDescription subpass_description{
            {},
            vk::PipelineBindPoint::eGraphics,
            {},
            color_attachment,
            {},
            &depth_attachment
    };
    vk::RenderPassCreateInfo renderPassCreateInfo{{}, attachment_descriptions, subpass_description};
    return {vk.device, renderPassCreateInfo};
}

std::vector<vk::raii::Framebuffer> make_framebuffers(VulkanContext& context) {
    GAME_ASSERT(*context.device);
    GAME_ASSERT(context.depth_image);
    GAME_ASSERT(*context.window.surface);
    GAME_ASSERT(*context.depth_image->view);
    GAME_ASSERT(*context.swapchain.swapchain);
    GAME_ASSERT(!context.swapchain.views.empty());

    std::array<vk::ImageView, 2> attachments;
    attachments[1] = *context.depth_image->view;

    vk::Extent2D extent = context.window.extent();
    vk::FramebufferCreateInfo create_info(
            {},
            *context.render_pass,
            attachments.size(), attachments.data(),
            extent.width, extent.height, 1
    );
    std::vector<vk::raii::Framebuffer> framebuffers;

    std::vector<vk::raii::ImageView>& views = context.swapchain.views;
    framebuffers.reserve(views.size());
    for (vk::raii::ImageView const& view: views) {
        attachments[0] = *view;
        framebuffers.emplace_back(context.device, create_info);
    }

    return framebuffers;
}

void create_swapchain(VulkanContext& vk) {
    Swapchain current = std::move(vk.swapchain);
    vk.swapchain = {vk, std::move(current)};
    vk.depth_image.emplace(vk);
    vk.render_pass = make_render_pass(vk);
    vk.framebuffers = make_framebuffers(vk);
}
