#include "render.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "app.hpp"
#include "shaders.hpp"
#include "shader_math.hpp"

void VulkanRenderPlugin::build(App& app) {
    app.globalCtx.emplace<VulkanContext>();
    app.globalCtx.emplace<WindowContext>(false, true, false);
}

void setupImgui(VulkanContext& vk) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(vk.surfData->window.handle, true);
    ImGui_ImplVulkan_InitInfo init_info{
            .Instance = static_cast<VkInstance>(**vk.inst),
            .PhysicalDevice = static_cast<VkPhysicalDevice>(**vk.physDev),
            .Device = static_cast<VkDevice>(**vk.device),
            .QueueFamily = vk.graphicsFamilyIdx,
            .Queue = static_cast<VkQueue>(**vk.graphicsQueue),
            .PipelineCache = static_cast<VkPipelineCache>(**vk.pipelineCache),
            .DescriptorPool = static_cast<VkDescriptorPool>(**vk.descriptorPool),
            .Subpass = 0,
            .MinImageCount = 2,
            .ImageCount = 2,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .Allocator = nullptr,
            .CheckVkResultFn = nullptr
    };
    ImGui_ImplVulkan_Init(&init_info, static_cast<VkRenderPass>(**vk.renderPass));
    std::cout << "[IMGUI] " << IMGUI_VERSION << " initialized" << std::endl;

    vk::raii::su::oneTimeSubmit(vk.cmdBufs->front(), *vk.graphicsQueue, [](vk::raii::CommandBuffer const& cmdBuf) {
        ImGui_ImplVulkan_CreateFontsTexture(static_cast<VkCommandBuffer>(*cmdBuf));
    });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void init(VulkanContext& vk) {
    std::string const appName = "Game Engine", engineName = "QEngine";
    vk.inst = vk::raii::su::makeInstance(vk.ctx, appName, engineName, {}, vk::su::getInstanceExtensions());
    std::cout << "[Vulkan] Instance created" << std::endl;

#if !defined(NDEBUG)
    vk.debugUtilMessenger = vk::raii::DebugUtilsMessengerEXT(*vk.inst, vk::su::makeDebugUtilsMessengerCreateInfoEXT());
#endif

    auto physDevs = vk::raii::PhysicalDevices{*vk.inst};
    if (physDevs.empty()) {
        throw std::runtime_error("No physical vk.devices found");
    }
    vk.physDev = std::move(physDevs.front());

    vk::PhysicalDeviceProperties const& props = vk.physDev->getProperties();
    std::cout << "[Vulkan]" << " Chose physical device: " << props.deviceName
              << std::endl;
    uint32_t apiVer = props.apiVersion;
    std::cout << "[Vulkan]" << " " << VK_VERSION_MAJOR(apiVer) << '.' << VK_VERSION_MINOR(apiVer) << '.' << VK_VERSION_PATCH(apiVer)
              << " device API version"
              << std::endl;

    // Creates window as well
    vk.surfData.emplace(*vk.inst, "Game Engine", vk::Extent2D(1280, 960));

    std::tie(vk.graphicsFamilyIdx, vk.graphicsFamilyIdx) = vk::raii::su::findGraphicsAndPresentQueueFamilyIndex(*vk.physDev, *vk.surfData->surface);

    std::vector<std::string> extensions = vk::su::getDeviceExtensions();
//#if !defined(NDEBUG)
//    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//#endif
    vk.device = vk::raii::su::makeDevice(*vk.physDev, vk.graphicsFamilyIdx, extensions);

    vk.cmdPool = vk::raii::CommandPool(*vk.device, {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, vk.graphicsFamilyIdx});
    vk.cmdBufs = vk::raii::CommandBuffers(*vk.device, {**vk.cmdPool, vk::CommandBufferLevel::ePrimary, 2});

    vk.graphicsQueue = vk::raii::Queue(*vk.device, vk.graphicsFamilyIdx, 0);
    vk.presentQueue = vk::raii::Queue(*vk.device, vk.presentFamilyIdx, 0);

    vk.drawFence = vk::raii::Fence(*vk.device, vk::FenceCreateInfo());
    vk.imgAcqSem = vk::raii::Semaphore(*vk.device, vk::SemaphoreCreateInfo());

    vk.pipelineCache = vk::raii::PipelineCache(*vk.device, vk::PipelineCacheCreateInfo());

    vk.descriptorPool = vk::raii::su::makeDescriptorPool(
            *vk.device, {
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

    createSwapChain(vk);

    setupImgui(vk);

    glslang::InitializeProcess();
}

void VulkanRenderPlugin::execute(App& app) {
    auto pVk = app.globalCtx.find<VulkanContext>();
    if (!pVk) return;

    VulkanContext& vk = *pVk;
    if (!vk.inst) init(vk);

    // Acquire next image and signal the semaphore
    auto [acqResult, curBuf] = vk.swapChainData->swapChain->acquireNextImage(vk::su::FenceTimeout, **vk.imgAcqSem, nullptr);

    if (acqResult == vk::Result::eSuboptimalKHR) {
        recreatePipeline(vk);
        return;
    }
    if (acqResult != vk::Result::eSuccess) {
        throw std::runtime_error("Invalid acquire next image KHR result");
    }
    if (vk.framebufs.size() <= curBuf) {
        throw std::runtime_error("Invalid framebuffer size");
    }

    for (auto [ent, shaderHandle]: app.renderWorld.view<const ShaderHandle>().each()) {
        if (vk.modelPipelines.contains(shaderHandle.value)) continue;

        // Create blank shader and fill it in
        createShaderPipeline(vk, vk.modelPipelines[shaderHandle.value]);
    }

    vk.cmdBufs->front().begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags()));
    vk::ClearValue clearColor = vk::ClearColorValue(std::array<float, 4>{0.2f, 0.2f, 0.2f, 0.2f});
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
    std::array<vk::ClearValue, 2> clearVals{clearColor, clearDepth};
    vk::RenderPassBeginInfo renderPassBeginInfo(
            **vk.renderPass,
            *vk.framebufs[curBuf],
            vk::Rect2D({}, vk.surfData->extent),
            clearVals
    );
    vk.cmdBufs->front().beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    renderOpaque(app);
    renderImGui(app);
    vk.cmdBufs->front().endRenderPass();
    vk.cmdBufs->front().end();

    vk::PipelineStageFlags waitDestStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    // Fences need to be manually reset
    vk.device->resetFences(**vk.drawFence);
    // Wait for the image to be acquired via the semaphore, signal the drawing fence when submitted
    vk.graphicsQueue->submit(vk::SubmitInfo(**vk.imgAcqSem, waitDestStageMask, *vk.cmdBufs->front()), **vk.drawFence);

    // Wait for the draw fence to be signaled
    while (vk::Result::eTimeout == vk.device->waitForFences(**vk.drawFence, VK_TRUE, vk::su::FenceTimeout));

    try {
        // Present frame to display
        vk::Result result = vk.presentQueue->presentKHR({nullptr, **vk.swapChainData->swapChain, curBuf});
        switch (result) {
            case vk::Result::eSuccess:
            case vk::Result::eSuboptimalKHR:
                break;
            default:
                throw std::runtime_error("Bad present KHR result: " + vk::to_string(result));
        }
    } catch (vk::OutOfDateKHRError const&) {
        recreatePipeline(vk);
    }

    glfwPollEvents();
    bool& keepOpen = app.globalCtx.at<WindowContext>().keepOpen;
    keepOpen = !glfwWindowShouldClose(vk.surfData->window.handle);
    if (!keepOpen) {
        vk.device->waitIdle();
    }
}

void VulkanRenderPlugin::cleanup(App& app) {
    ImGui_ImplVulkan_Shutdown();
    glslang::FinalizeProcess();
    for (auto& [_, pipeline]: app.globalCtx.at<VulkanContext>().modelPipelines) {
        for (auto& item: pipeline.shaders) {
            spvReflectDestroyShaderModule(&item.reflect);
        }
    }
}
