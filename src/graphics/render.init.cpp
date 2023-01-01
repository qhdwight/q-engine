#define VMA_IMPLEMENTATION

#include "render.hpp"

#include "render.debug.hpp"

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

constexpr std::string_view APP_NAME = "Game Engine"sv;
constexpr uint32_t APP_VERSION = 1u;
constexpr std::string_view ENGINE_NAME = "QEngine"sv;
constexpr uint32_t ENGINE_VERSION = 1u;

[[nodiscard]] Extensions get_extensions(VulkanContext const& vulkan) {
    Extensions desired_extensions{VK_KHR_SURFACE_EXTENSION_NAME};
    if constexpr (OS == OS::Windows) {
        desired_extensions.push_back("VK_KHR_win32_surface");
    } else if constexpr (OS == OS::Linux) {
        desired_extensions.push_back("VK_KHR_xcb_surface");
    } else if constexpr (OS == OS::macOS) {
        // TODO: make defines
        desired_extensions.push_back("VK_EXT_metal_surface");
        desired_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        desired_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    };
    if constexpr (IS_DEBUG) {
        desired_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    std::vector<vk::ExtensionProperties> available_extensions = vulkan.ctx.enumerateInstanceExtensionProperties();

    std::vector<std::string_view> sorted_desired_extensions(desired_extensions.size());
    std::ranges::partial_sort_copy(desired_extensions | std::views::transform([](const char* s) { return std::string_view{s}; }),
                                   sorted_desired_extensions);
    std::vector<std::string_view> sorted_available_extensions(available_extensions.size());
    std::ranges::partial_sort_copy(available_extensions | std::views::transform([](const auto& e) { return std::string_view{e.extensionName}; }),
                                   sorted_available_extensions);

    std::vector<std::string_view> missing_extensions;
    std::ranges::set_difference(sorted_desired_extensions, sorted_available_extensions, std::back_inserter(missing_extensions));

    std::cout << "[Vulkan] Available extensions:\n";
    for (std::string_view name: sorted_available_extensions) {
        std::cout << std::format("\t{}\n", name);
    }

    if (!missing_extensions.empty()) {
        std::ostringstream out;
        out << "[Vulkan] Missing extensions:\n";
        for (std::string_view name: missing_extensions) {
            out << std::format("\t{}\n", name);
        }
        throw std::runtime_error(out.str());
    }

    return desired_extensions;
}

[[nodiscard]] DeviceExtensions get_device_extensions() {
    DeviceExtensions extensions;
    extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    if constexpr (OS == OS::macOS) {
        extensions.push_back("VK_KHR_portability_subset");
    }
    if constexpr (IS_DEBUG && OS != OS::macOS) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

auto make_instance_create_info(vk::ApplicationInfo const& app_info, Layers const& layers, Extensions const& extensions) {
    vk::InstanceCreateInfo create_info{{}, &app_info, layers, extensions};
    if constexpr (OS == OS::macOS) create_info.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    if constexpr (IS_DEBUG) {
        using
        enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
        using
        enum vk::DebugUtilsMessageTypeFlagBitsEXT;
        return vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>{
                create_info,
                {{}, eWarning | eError, eGeneral | ePerformance | eValidation, &debugUtilsMessengerCallback},
        };
    } else {
        return vk::StructureChain<vk::InstanceCreateInfo>{create_info};
    }
}

void fill_instance(VulkanContext& vk) {
    vk::ApplicationInfo app_info{APP_NAME.data(), APP_VERSION, ENGINE_NAME.data(), ENGINE_VERSION, VK_API_VERSION_1_3};
    Extensions extensions = get_extensions(vk);
    Layers layers;

    auto create_info = make_instance_create_info(app_info, layers, extensions);
    vk.inst = {vk.ctx, create_info.get<vk::InstanceCreateInfo>()};

    if constexpr (IS_DEBUG) {
        vk.debug_util_messenger = {vk.inst, create_info.get<vk::DebugUtilsMessengerCreateInfoEXT>()};
    }
}

struct PhysicalDeviceComparator {
    uint32_t score(vk::raii::PhysicalDevice const& device) const {
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

    auto physical_devices = vk::raii::PhysicalDevices{vk.inst};
    if (physical_devices.empty()) {
        throw std::runtime_error("No physical devices found");
    }

    vk::raii::PhysicalDevice physical_device = std::move(*std::ranges::max_element(physical_devices, PhysicalDeviceComparator{}));

    vk::PhysicalDeviceProperties const& properties = physical_device.getProperties();
    std::cout << std::format("[Vulkan] Chose physical device {}\n", properties.deviceName.data());
    uint32_t api_version = properties.apiVersion;
    std::cout << std::format("[Vulkan] {}.{}.{} device API version\n",
                             VK_VERSION_MAJOR(api_version), VK_VERSION_MINOR(api_version), VK_VERSION_PATCH(api_version));
    return physical_device;
}

std::pair<uint32_t, uint32_t> find_queue_indices(VulkanContext const& vk) {
    GAME_ASSERT(*vk.window.surface);

    using QueueIndex = uint32_t;
    using Queue = std::tuple<QueueIndex, vk::QueueFamilyProperties>;

    auto queue_properties = vk.physical_device.getQueueFamilyProperties();
    auto queues = std::views::zip(std::views::iota(QueueIndex{0}), queue_properties);

    auto graphics_queues = queues | std::views::filter([](Queue const& queue) {
        return static_cast<bool>(std::get<vk::QueueFamilyProperties>(queue).queueFlags & vk::QueueFlagBits::eGraphics);
    }) | std::views::transform([](Queue const& queue) { return std::get<QueueIndex>(queue); });
    if (graphics_queues.empty()) throw std::runtime_error("No graphics queues found");

    auto presentation_queues = queues | std::views::filter([&](Queue const& queue) {
        return vk.physical_device.getSurfaceSupportKHR(std::get<QueueIndex>(queue), *vk.window.surface);
    }) | std::views::transform([](Queue const& queue) { return std::get<QueueIndex>(queue); });
    if (presentation_queues.empty()) throw std::runtime_error("No presentation queues found");

    std::vector<QueueIndex> combined;
    std::ranges::set_intersection(graphics_queues, presentation_queues, std::back_inserter(combined));
    if (!combined.empty()) return {combined.front(), combined.front()};

    return {graphics_queues.front(), presentation_queues.front()};
}

vk::raii::Device make_device(VulkanContext const& vulkan) {
    GAME_ASSERT(*vulkan.physical_device);

    DeviceExtensions extensions = get_device_extensions();
    float queue_priority = 0.0f;
    vk::DeviceQueueCreateInfo queue_create_info({}, vulkan.graphics_family_index, 1, &queue_priority);
    vk::DeviceCreateInfo create_info{{}, queue_create_info, {}, extensions};
    return {vulkan.physical_device, create_info};
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
    GAME_ASSERT(*vk.physical_device);

    VmaVulkanFunctions functions{
            .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = &vkGetDeviceProcAddr,
    };

    VmaAllocatorCreateInfo create_info{
            .physicalDevice = static_cast<VkPhysicalDevice>(*vk.physical_device),
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

void init(VulkanContext& vk) {
    fill_instance(vk);

    vk.physical_device = make_physical_device(vk);

    vk.window = {vk.inst, "Window"sv, vk::Extent2D{800, 600}};

    std::tie(vk.graphics_family_index, vk.present_family_index) = find_queue_indices(vk);

    vk.device = make_device(vk);

    vk.allocator = {vk};

    vk.command_pool = {vk.device, {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, vk.graphics_family_index}};
    vk.command_buffers = {vk.device, {*vk.command_pool, vk::CommandBufferLevel::ePrimary, 2}};

    vk.graphics_queue = {vk.device, vk.graphics_family_index, 0};
    vk.present_queue = {vk.device, vk.present_family_index, 0};

    vk.draw_fence = {vk.device, {}};
    vk.image_acquire_semaphore = {vk.device, {}};

    vk.pipeline_cache = {vk.device, {}};

    vk.descriptor_pool = make_descriptor_pool(
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
            }
    );

    create_swapchain(vk);

//    createSwapChain(vk);
//
//    setupImgui(vk);
//
//    glslang::InitializeProcess();
}
