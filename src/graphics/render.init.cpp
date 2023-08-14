#define VMA_IMPLEMENTATION

#include "render.hpp"

#include "render.debug.hpp"

constexpr std::string_view APP_NAME = "Game Engine"sv;
constexpr uint32_t APP_VERSION = 1u;
constexpr std::string_view ENGINE_NAME = "QEngine"sv;
constexpr uint32_t ENGINE_VERSION = 1u;

[[nodiscard]] InstanceExtensions get_instance_extensions(VulkanContext const& vulkan) {
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

    std::vector<vk::ExtensionProperties> availableExtensions = vulkan.ctx.enumerateInstanceExtensionProperties();

    std::vector<std::string_view> sorted_desired_extensions(desiredExtensions.size());
    std::ranges::partial_sort_copy(desiredExtensions | std::views::transform([](const char* s) { return std::string_view{s}; }),
                                   sorted_desired_extensions);
    std::vector<std::string_view> sorted_available_extensions(availableExtensions.size());
    std::ranges::partial_sort_copy(availableExtensions | std::views::transform([](const auto& e) { return std::string_view{e.extensionName}; }),
                                   sorted_available_extensions);

    std::vector<std::string_view> missing_extensions;
    std::ranges::set_difference(sorted_desired_extensions, sorted_available_extensions, std::back_inserter(missing_extensions));

    log("[Vulkan] Available instance extensions:");
    for (std::string_view extension: sorted_available_extensions) {
        log("\t{}", extension);
    }

    if (!missing_extensions.empty()) {
        std::ostringstream out;
        out << "[Vulkan] Missing extensions:\n";
        for (std::string_view extension: missing_extensions) {
            out << std::format("\t{}", extension);
        }
        throw std::runtime_error(out.str());
    }

    return desiredExtensions;
}

[[nodiscard]] DeviceExtensions get_device_extensions() {
    DeviceExtensions extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};
    if constexpr (OS == OS::macOS) {
        extensions.push_back("VK_KHR_portability_subset");
    }
    return extensions;
}

auto make_instance_create_info(vk::ApplicationInfo const& app_info, Layers const& layers, InstanceExtensions const& extensions) {
    vk::InstanceCreateInfo createInfo{vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR, &app_info, layers, extensions};
    if constexpr (IS_DEBUG) {
        using
        enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
        using
        enum vk::DebugUtilsMessageTypeFlagBitsEXT;
        return vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>{
                createInfo,
                {{}, eWarning | eError, eGeneral | ePerformance | eValidation, &debugUtilsMessengerCallback},
        };
    } else {
        return vk::StructureChain<vk::InstanceCreateInfo>{createInfo};
    }
}

void fill_instance(VulkanContext& vk) {
    vk::ApplicationInfo appInfo{APP_NAME.data(), APP_VERSION, ENGINE_NAME.data(), ENGINE_VERSION, VK_API_VERSION_1_3};
    InstanceExtensions extensions = get_instance_extensions(vk);
    Layers layers;
    if constexpr (IS_DEBUG) {
        layers.emplace_back("VK_LAYER_KHRONOS_validation");
    }

    log("[Vulkan] Available instance layers:");
    for (auto const& layer: vk::enumerateInstanceLayerProperties()) {
        log("\t{}", std::string_view{layer.layerName});
    }

    auto createInfo = make_instance_create_info(appInfo, layers, extensions);
    vk.inst = {vk.ctx, createInfo.get<vk::InstanceCreateInfo>()};

    if constexpr (IS_DEBUG) {
        vk.debugUtilMessenger = {vk.inst, createInfo.get<vk::DebugUtilsMessengerCreateInfoEXT>()};
    }
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

vk::raii::PhysicalDevice make_physical_device(VulkanContext const& vk) {
    GAME_ASSERT(*vk.inst);

    auto physicalDevices = vk::raii::PhysicalDevices{vk.inst};
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

std::pair<uint32_t, uint32_t> find_queue_indices(VulkanContext const& vk) {
    GAME_ASSERT(*vk.window.surface);

    using QueueIndex = uint32_t;
    using Queue = std::tuple<QueueIndex, vk::QueueFamilyProperties>;

    auto queue_properties = vk.physicalDevice.getQueueFamilyProperties();
    auto queues = std::views::zip(std::views::iota(QueueIndex{0}), queue_properties);

    auto graphics_queues = queues | std::views::filter([](Queue const& queue) {
        return static_cast<bool>(std::get<vk::QueueFamilyProperties>(queue).queueFlags & vk::QueueFlagBits::eGraphics);
    }) |
                           std::views::transform([](Queue const& queue) { return std::get<QueueIndex>(queue); });
    if (graphics_queues.empty()) throw std::runtime_error("No graphics queues found");

    auto presentation_queues = queues | std::views::filter([&](Queue const& queue) {
        return vk.physicalDevice.getSurfaceSupportKHR(std::get<QueueIndex>(queue), *vk.window.surface);
    }) |
                               std::views::transform([](Queue const& queue) { return std::get<QueueIndex>(queue); });
    if (presentation_queues.empty()) throw std::runtime_error("No presentation queues found");

    std::vector<QueueIndex> combined;
    std::ranges::set_intersection(graphics_queues, presentation_queues, std::back_inserter(combined));
    if (!combined.empty()) return {combined.front(), combined.front()};

    return {graphics_queues.front(), presentation_queues.front()};
}

vk::raii::Device make_logical_device(VulkanContext const& vk) {
    GAME_ASSERT(*vk.physicalDevice);

    DeviceExtensions extensions = get_device_extensions();
    float queue_priority = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo({}, vk.graphicsFamilyIndex, 1, &queue_priority);
    vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{true};
    vk::DeviceCreateInfo createInfo{{}, queueCreateInfo, {}, extensions, {}, &dynamicRenderingFeatures};
    return {vk.physicalDevice, createInfo};
}

vk::raii::DescriptorPool make_descriptor_pool(vk::raii::Device const& device, std::vector<vk::DescriptorPoolSize> const& pool_sizes) {
    GAME_ASSERT(!pool_sizes.empty());
    uint32_t max_sets = std::accumulate(pool_sizes.begin(), pool_sizes.end(), 0,
                                        [](uint32_t sum, vk::DescriptorPoolSize const& dps) { return sum + dps.descriptorCount; });
    GAME_ASSERT(max_sets > 0);
    vk::DescriptorPoolCreateInfo create_info{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, max_sets, pool_sizes};
    return {device, create_info};
}

MemAllocator::MemAllocator(VulkanContext& vk) {
    GAME_ASSERT(*vk.inst);
    GAME_ASSERT(*vk.device);
    GAME_ASSERT(*vk.physicalDevice);

    VmaVulkanFunctions functions{
            .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = &vkGetDeviceProcAddr,
    };

    VmaAllocatorCreateInfo create_info{
            .physicalDevice = static_cast<VkPhysicalDevice>(*vk.physicalDevice),
            .device = static_cast<VkDevice>(*vk.device),
            .pVulkanFunctions = &functions,
            .instance = static_cast<VkInstance>(*vk.inst),
            .vulkanApiVersion = VK_API_VERSION_1_3,
    };

    auto* raw = new VmaAllocator{};
    if (vmaCreateAllocator(&create_info, raw) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create memory allocator");
    }
    reset(raw, [](VmaAllocator* p) {
        vmaDestroyAllocator(*p);
        delete p;
    });
}

template<typename Func>
void one_time_submit(vk::raii::CommandBuffer const& command_buffer, vk::raii::Queue const& queue, Func const& func) {
    command_buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    func(command_buffer);
    command_buffer.end();
    queue.submit(vk::SubmitInfo{nullptr, nullptr, *command_buffer}, nullptr);
    queue.waitIdle();
}

void setup_imgui(VulkanContext& vk) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(vk.window.windowHandle.get(), true);
    ImGui_ImplVulkan_InitInfo init_info{
            .Instance = static_cast<VkInstance>(*vk.inst),
            .PhysicalDevice = static_cast<VkPhysicalDevice>(*vk.physicalDevice),
            .Device = static_cast<VkDevice>(*vk.device),
            .QueueFamily = vk.graphicsFamilyIndex,
            .Queue = static_cast<VkQueue>(*vk.graphicsQueue),
            .PipelineCache = static_cast<VkPipelineCache>(*vk.pipelineCache),
            .DescriptorPool = static_cast<VkDescriptorPool>(*vk.descriptorPool),
            .Subpass = 0,
            .MinImageCount = 2,
            .ImageCount = 2,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .UseDynamicRendering = true,
            .ColorAttachmentFormat = static_cast<VkFormat>(vk.swapchain.format.format),
            .Allocator = nullptr,
            .CheckVkResultFn = nullptr,
    };
    ImGui_ImplVulkan_Init(&init_info, {});
    std::cout << "[IMGUI] " << IMGUI_VERSION << " initialized" << std::endl;

    one_time_submit(vk.commandBuffers.front(), vk.graphicsQueue, [](vk::raii::CommandBuffer const& commands) {
        ImGui_ImplVulkan_CreateFontsTexture(static_cast<VkCommandBuffer>(*commands));
    });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void init(VulkanContext& vk) {
    glfwInit();
    std::cout << std::format("[GLFW] {} initialized\n", glfwGetVersionString());
    glfwSetErrorCallback([](int error, const char* msg) {
        std::cerr << std::format("[GLFW] Error {}: {}\n", error, msg);
    });
    if (!glfwVulkanSupported()) throw std::runtime_error("[GLFW] Vulkan is not supported");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    fill_instance(vk);

    vk.physicalDevice = make_physical_device(vk);

    vk.window = {vk.inst, "Window"sv, vk::Extent2D{800, 600}};

    std::tie(vk.graphicsFamilyIndex, vk.presentFamilyIndex) = find_queue_indices(vk);

    vk.device = make_logical_device(vk);

    vk.allocator = {vk};

    vk.commandPool = {vk.device, {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, vk.graphicsFamilyIndex}};
    vk.commandBuffers = {vk.device, {*vk.commandPool, vk::CommandBufferLevel::ePrimary, 2}};

    vk.graphicsQueue = {vk.device, vk.graphicsFamilyIndex, 0};
    vk.presentQueue = {vk.device, vk.presentFamilyIndex, 0};

    for (uint32_t i = 0; i < vk.commandBuffers.size(); ++i) {
        vk.presentationCompleteSemaphores.emplace_back(vk.device, vk::SemaphoreCreateInfo{});
        vk.renderCompleteSemaphores.emplace_back(vk.device, vk::SemaphoreCreateInfo{});
        vk.waitFences.emplace_back(vk.device, vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled});
    }

    vk.pipelineCache = {vk.device, vk::PipelineCacheCreateInfo{}};

    vk.descriptorPool = make_descriptor_pool(
            vk.device, {
                    {vk::DescriptorType::eSampler,              64},
                    {vk::DescriptorType::eCombinedImageSampler, 64},
                    {vk::DescriptorType::eSampledImage,         64},
                    {vk::DescriptorType::eStorageImage,         64},
                    {vk::DescriptorType::eUniformTexelBuffer,   64},
                    {vk::DescriptorType::eStorageTexelBuffer,   64},
                    {vk::DescriptorType::eUniformBuffer,        64},
                    {vk::DescriptorType::eStorageBuffer,        64},
                    {vk::DescriptorType::eUniformBufferDynamic, 64},
                    {vk::DescriptorType::eStorageBufferDynamic, 64},
                    {vk::DescriptorType::eInputAttachment,      64},
            });

    createSwapchain(vk);

    setup_imgui(vk);
    //
    //    glslang::InitializeProcess();
}
