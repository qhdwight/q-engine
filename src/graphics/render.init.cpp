#include "render.hpp"

#include "render.debug.hpp"

constexpr std::string_view APP_NAME = "Game Engine"sv;
constexpr uint32_t APP_VERSION = 1u;
constexpr std::string_view ENGINE_NAME = "QEngine"sv;
constexpr uint32_t ENGINE_VERSION = 1u;

[[nodiscard]] Extensions getExtensions() {
    Extensions extensions{VK_KHR_SURFACE_EXTENSION_NAME};
    if constexpr (OS == OS::Windows) {
        extensions.push_back("VK_KHR_win32_surface"sv);
    } else if constexpr (OS == OS::Linux) {
        extensions.push_back("VK_KHR_xcb_surface"sv);
    } else if constexpr (OS == OS::macOS) {
        // TODO: make defines
        extensions.push_back("VK_EXT_metal_surface"sv);
        extensions.push_back("VK_KHR_get_physical_device_properties2"sv);
    };
    return extensions;
}

auto makeInstanceCreateInfo(vk::ApplicationInfo const &appInfo, Layers const &layers, Extensions const &extensions) {
    if constexpr (IS_DEBUG) {
        return vk::StructureChain<vk::InstanceCreateInfo>{{{}, &appInfo, layers, extensions}};
    } else {
        auto severityFlags =
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        auto messageTypeFlags =
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
        return vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>{
                vk::InstanceCreateInfo{
                        .pApplicationInfo = &appInfo,
                        .ppEnabledLayerNames =layers, .ppEnabledExtensionNames =    extensions},
                vk::DebugUtilsMessengerCreateInfoEXT{{}, severityFlags, messageTypeFlags, &debugUtilsMessengerCallback}
        };
    }
}

vk::raii::Instance make_instance(VulkanContext &vulkan) {
    vk::ApplicationInfo appInfo{
            .pApplicationName = APP_NAME.data(),
            .applicationVersion = APP_VERSION,
            .pEngineName = ENGINE_NAME.data(),
            .engineVersion = ENGINE_VERSION,
            .apiVersion = VK_VERSION_1_3,
    };
    vulkan.appInfo = appInfo;

    auto instanceCreateInfo = makeInstanceCreateInfo(vulkan.appInfo, {}, getExtensions());
    return {vulkan.ctx, instanceCreateInfo.get<vk::InstanceCreateInfo>()};
}

void init(VulkanContext &vk) {

    vk::raii::Instance instance = make_instance(vk);

//    std::string const appName = "Game Engine", engineName = "QEngine";
//    vk.inst = vk::raii::su::makeInstance(vk.ctx, appName, engineName, {}, vk::su::getInstanceExtensions());
//    std::cout << "[Vulkan] Instance created" << std::endl;
//
//#if !defined(NDEBUG)
//    vk.debugUtilMessenger = vk::raii::DebugUtilsMessengerEXT(*vk.inst, vk::su::makeDebugUtilsMessengerCreateInfoEXT());
//#endif
//
//    auto physDevs = vk::raii::PhysicalDevices{*vk.inst};
//    if (physDevs.empty()) {
//        throw std::runtime_error("No physical vk.devices found");
//    }
//    vk.physDev = std::move(physDevs.front());
//
//    vk::PhysicalDeviceProperties const &props = vk.physDev->getProperties();
//    std::cout << "[Vulkan]" << " Chose physical device: " << props.deviceName
//              << std::endl;
//    uint32_t apiVer = props.apiVersion;
//    std::cout << "[Vulkan]" << " " << VK_VERSION_MAJOR(apiVer) << '.' << VK_VERSION_MINOR(apiVer) << '.'
//              << VK_VERSION_PATCH(apiVer)
//              << " device API version"
//              << std::endl;
//
//    // Creates window as well
//    vk.surfData.emplace(*vk.inst, "Game Engine", vk::Extent2D(1280, 960));
//
//    std::tie(vk.graphicsFamilyIdx, vk.graphicsFamilyIdx) = vk::raii::su::findGraphicsAndPresentQueueFamilyIndex(
//            *vk.physDev, *vk.surfData->surface);
//
//    std::vector<std::string> extensions = vk::su::getDeviceExtensions();
////#if !defined(NDEBUG)
////    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
////#endif
//    vk.device = vk::raii::su::makeDevice(*vk.physDev, vk.graphicsFamilyIdx, extensions);
//
//    vk.cmdPool = vk::raii::CommandPool(*vk.device,
//                                       {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, vk.graphicsFamilyIdx});
//    vk.cmdBufs = vk::raii::CommandBuffers(*vk.device, {**vk.cmdPool, vk::CommandBufferLevel::ePrimary, 2});
//
//    vk.graphicsQueue = vk::raii::Queue(*vk.device, vk.graphicsFamilyIdx, 0);
//    vk.presentQueue = vk::raii::Queue(*vk.device, vk.presentFamilyIdx, 0);
//
//    vk.drawFence = vk::raii::Fence(*vk.device, vk::FenceCreateInfo());
//    vk.imgAcqSem = vk::raii::Semaphore(*vk.device, vk::SemaphoreCreateInfo());
//
//    vk.pipelineCache = vk::raii::PipelineCache(*vk.device, vk::PipelineCacheCreateInfo());
//
//    vk.descriptorPool = vk::raii::su::makeDescriptorPool(
//            *vk.device, {
//                    {vk::DescriptorType::eSampler,              64},
//                    {vk::DescriptorType::eCombinedImageSampler, 64},
//                    {vk::DescriptorType::eSampledImage,         64},
//                    {vk::DescriptorType::eStorageImage,         64},
//                    {vk::DescriptorType::eUniformTexelBuffer,   64},
//                    {vk::DescriptorType::eStorageTexelBuffer,   64},
//                    {vk::DescriptorType::eUniformBuffer,        64},
//                    {vk::DescriptorType::eStorageBuffer,        64},
//                    {vk::DescriptorType::eUniformBufferDynamic, 64},
//                    {vk::DescriptorType::eStorageBufferDynamic, 64},
//                    {vk::DescriptorType::eInputAttachment,      64},
//            }
//    );
//
//    createSwapChain(vk);
//
//    setupImgui(vk);
//
//    glslang::InitializeProcess();
}
