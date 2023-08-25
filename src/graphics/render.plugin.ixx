export module render:plugin;

import :init;
import :memory;
import :present;

import app;
import game;
import logging;

import std;
import glfw;
import imgui;
import vulkan;

using namespace std::literals;

constexpr std::uint64_t FENCE_TIMEOUT = 100'000'000;

export struct WindowContext {
    bool isReady;
    bool keepOpen;
    bool isFocused;
};

struct VulkanContext {
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::DispatchLoaderDynamic dispatchLoader;
    vk::raii::DebugUtilsMessengerEXT debugUtilMessenger = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    Window window;
    vk::raii::Device device = nullptr;
    QueueFamilyIndices queueFamilyIndices;
    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::Queue presentQueue = nullptr;
    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::CommandBuffers commandBuffers = nullptr;
    MemAllocator allocator;
    Swapchain swapchain;
    std::optional<DepthImage> depthImage;
    //    std::unordered_map<asset_handle_t, vk::raii::su::TextureData> textures;
    //    std::unordered_map<asset_handle_t, ModelBuffers> modelBufData;
    //    std::unordered_map<asset_handle_t, CubeMapData> cubeMaps;
    //    aligned_vector<Material> materialUpload;
    //    aligned_vector<ModelUpload> modelUpload;
    vk::raii::DescriptorPool descriptorPool = nullptr;
    vk::raii::PipelineCache pipelineCache = nullptr;
    //    std::unordered_map<asset_handle_t, Pipeline> modelPipelines;
    std::vector<vk::raii::Semaphore> presentationCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderCompleteSemaphores;
    std::vector<vk::raii::Fence> waitFences;
    //    CameraUpload cameraUpload;
    //    SceneUpload sceneUpload;
    std::uint32_t currentFrame = 0;
    std::uint32_t currentSwapchainImageIndex = 0;
};

void createSwapchain(VulkanContext& context) {
    Swapchain current = std::move(context.swapchain);
    context.swapchain = {context.device, context.physicalDevice, context.queueFamilyIndices, context.window, std::move(current)};
    context.depthImage.emplace(context.device, context.allocator, context.window.extent());
}

void recreatePipeline(VulkanContext& context) {
    context.device.waitIdle();
    createSwapchain(context);
}

void setupImgui(VulkanContext& context) {
    ImGui::checkVersion();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(context.window.windowHandle.get(), true);
    ImGui_ImplVulkan_InitInfo initInfo{
            .Instance = static_cast<VkInstance>(*context.instance),
            .PhysicalDevice = static_cast<VkPhysicalDevice>(*context.physicalDevice),
            .Device = static_cast<VkDevice>(*context.device),
            .QueueFamily = context.queueFamilyIndices.graphicsFamilyIndex,
            .Queue = static_cast<VkQueue>(*context.graphicsQueue),
            .PipelineCache = static_cast<VkPipelineCache>(*context.pipelineCache),
            .DescriptorPool = static_cast<VkDescriptorPool>(*context.descriptorPool),
            .Subpass = 0,
            .MinImageCount = 2,
            .ImageCount = 2,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .UseDynamicRendering = true,
            .ColorAttachmentFormat = static_cast<VkFormat>(context.swapchain.format.format),
            .Allocator = nullptr,
            .CheckVkResultFn = nullptr,
    };
    ImGui_ImplVulkan_Init(&initInfo, {});
    std::cout << "[IMGUI] " << ImGui::VERSION << " initialized" << std::endl;

    oneTimeSubmit(context.commandBuffers.front(), context.graphicsQueue, [](vk::raii::CommandBuffer const& commands) {
        ImGui_ImplVulkan_CreateFontsTexture(static_cast<VkCommandBuffer>(*commands));
    });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

//void renderImGuiOverlay(App& app) {
//    auto& diagnostics = app.globalContext.get<DiagnosticResource>();
//    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
//                                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
//    static bool open = true;
//    static int corner = 0;
//    if (corner != -1) {
//        const float PAD = 10.0f;
//        const ImGuiViewport* viewport = ImGui::GetMainViewport();
//        ImVec2 work_pos = viewport->WorkPos;// Use work area to avoid menu-bar/task-bar, if any!
//        ImVec2 work_size = viewport->WorkSize;
//        ImVec2 window_pos, window_pos_pivot;
//        window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
//        window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
//        window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
//        window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
//        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
//        windowFlags |= ImGuiWindowFlags_NoMove;
//    }
//    ImGui::SetNextWindowBgAlpha(0.35f);// Transparent background
//    if (ImGui::Begin("Diagnostics", &open, windowFlags)) {
//        clock_delta_t avgFrameTime = diagnostics.getAvgFrameTime();
//        ImGui::Text("%.3f ms/frame (%.1f FPS)", ms_t(avgFrameTime).count(), 1.0 / sec_t(avgFrameTime).count());
//        if (ImGui::BeginPopupContextWindow()) {
//            if (ImGui::MenuItem("Custom", nullptr, corner == -1)) corner = -1;
//            if (ImGui::MenuItem("Top-left", nullptr, corner == 0)) corner = 0;
//            if (ImGui::MenuItem("Top-right", nullptr, corner == 1)) corner = 1;
//            if (ImGui::MenuItem("Bottom-left", nullptr, corner == 2)) corner = 2;
//            if (ImGui::MenuItem("Bottom-right", nullptr, corner == 3)) corner = 3;
//            if (open && ImGui::MenuItem("Close")) open = false;
//            ImGui::EndPopup();
//        }
//    }
//    ImGui::End();
//}

void renderImgui(App& app) {
    auto& context = app.globalContext.get<VulkanContext>();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(*context.commandBuffers[context.currentFrame]));
}

export class VulkanRenderPlugin : public Plugin {
public:
    void build(App& app) override {
        app.globalContext.emplace<WindowContext>(false, true, false);

        setupGlfw();
        auto& context = app.globalContext.emplace<VulkanContext>();
        context.instance = makeInstance(context.context);
        if constexpr (IS_DEBUG) {
            context.debugUtilMessenger = makeDebugUtilsMessenger(context.instance);
        }
        context.physicalDevice = makePhysicalDevice(context.instance);
        context.window = {context.instance, "Window", vk::Extent2D{800, 600}};
        context.queueFamilyIndices = findQueueFamilyIndices(context.physicalDevice, context.window.surface);
        context.device = makeLogicalDevice(context.physicalDevice, context.queueFamilyIndices.graphicsFamilyIndex);
        context.allocator = {context.instance, context.device, context.physicalDevice};
        context.commandPool = {context.device, {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, context.queueFamilyIndices.graphicsFamilyIndex}};
        context.commandBuffers = {context.device, {*context.commandPool, vk::CommandBufferLevel::ePrimary, 2}};
        context.graphicsQueue = {context.device, context.queueFamilyIndices.graphicsFamilyIndex, 0};
        context.presentQueue = {context.device, context.queueFamilyIndices.presentFamilyIndex, 0};
        for (std::uint32_t i = 0; i < context.commandBuffers.size(); ++i) {
            context.presentationCompleteSemaphores.emplace_back(context.device, vk::SemaphoreCreateInfo{});
            context.renderCompleteSemaphores.emplace_back(context.device, vk::SemaphoreCreateInfo{});
            context.waitFences.emplace_back(context.device, vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled});
        }
        context.pipelineCache = {context.device, vk::PipelineCacheCreateInfo{}};
        context.descriptorPool = makeDescriptorPool(
                context.device, {
                                        {vk::DescriptorType::eSampler, 64},
                                        {vk::DescriptorType::eCombinedImageSampler, 64},
                                        {vk::DescriptorType::eSampledImage, 64},
                                        {vk::DescriptorType::eStorageImage, 64},
                                        {vk::DescriptorType::eUniformTexelBuffer, 64},
                                        {vk::DescriptorType::eStorageTexelBuffer, 64},
                                        {vk::DescriptorType::eUniformBuffer, 64},
                                        {vk::DescriptorType::eStorageBuffer, 64},
                                        {vk::DescriptorType::eUniformBufferDynamic, 64},
                                        {vk::DescriptorType::eStorageBufferDynamic, 64},
                                        {vk::DescriptorType::eInputAttachment, 64},
                                });
        createSwapchain(context);
        setupImgui(context);
    }

    void execute(App& app) override {
        auto& context = app.globalContext.get<VulkanContext>();

        check(*context.device);
        check(*context.swapchain.swapchain);
        check(!context.commandBuffers.empty());

        try {
            vk::raii::Fence const& fence = context.waitFences[context.currentFrame];
            vk::raii::Semaphore const& presentationComplete = context.presentationCompleteSemaphores[context.currentFrame];
            vk::raii::Semaphore const& renderComplete = context.renderCompleteSemaphores[context.currentFrame];
            vk::raii::CommandBuffer const& cmdBuffer = context.commandBuffers[context.currentFrame];
            check(*fence);
            check(*presentationComplete);
            check(*renderComplete);
            check(*cmdBuffer);

            vk::Result waitResult = context.device.waitForFences(*fence, true, FENCE_TIMEOUT);
            check(waitResult == vk::Result::eSuccess);

            vk::Result acquireResult;
            std::tie(acquireResult, context.currentSwapchainImageIndex) = context.swapchain.swapchain.acquireNextImage(FENCE_TIMEOUT, *presentationComplete, nullptr);
            check(acquireResult == vk::Result::eSuccess);

            //
            //    for (auto [ent, shaderHandle]: app.renderWorld.view<const ShaderHandle>().each()) {
            //        if (vk.modelPipelines.contains(shaderHandle.value)) continue;
            //
            //        // Create blank shader and fill it in
            //        createShaderPipeline(vk, vk.modelPipelines[shaderHandle.value]);
            //    }
            //

            context.device.resetFences(*fence);

            cmdBuffer.reset();
            cmdBuffer.begin({});

            cmdBuffer.pipelineBarrier(
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::PipelineStageFlagBits::eTopOfPipe,
                    {},
                    {},
                    {},
                    vk::ImageMemoryBarrier{
                            vk::AccessFlagBits::eColorAttachmentWrite,
                            {},
                            vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eColorAttachmentOptimal,
                            {},
                            {},
                            context.swapchain.swapchain.getImages()[context.currentSwapchainImageIndex],
                            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
                    });
            cmdBuffer.pipelineBarrier(
                    vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
                    vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
                    {},
                    {},
                    {},
                    vk::ImageMemoryBarrier{
                            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                            {},
                            vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eDepthStencilAttachmentOptimal,
                            {},
                            {},
                            context.depthImage->image,
                            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1},
                    });

            {
                vk::RenderingAttachmentInfo colorAttachmentInfo{
                        *context.swapchain.views[context.currentSwapchainImageIndex],
                        vk::ImageLayout::eAttachmentOptimalKHR,
                        vk::ResolveModeFlagBits::eNone,
                        {},
                        {},
                        vk::AttachmentLoadOp::eClear,
                        vk::AttachmentStoreOp::eStore,
                        vk::ClearColorValue{0.2f, 0.2f, 0.2f, 0.2f},
                };
                vk::RenderingAttachmentInfo depthAttachmentInfo{
                        *context.depthImage->view,
                        vk::ImageLayout::eAttachmentOptimalKHR,
                        vk::ResolveModeFlagBits::eNone,
                        {},
                        {},
                        vk::AttachmentLoadOp::eClear,
                        vk::AttachmentStoreOp::eStore,
                        vk::ClearDepthStencilValue(1.0f, 0),
                };
                vk::RenderingInfoKHR renderingInfo{
                        {},
                        vk::Rect2D{{}, context.swapchain.extent},
                        1,
                        {},
                        colorAttachmentInfo,
                        //                        &depthAttachmentInfo,
                };

                cmdBuffer.beginRenderingKHR(renderingInfo);
                //                renderOpaque(app);
                renderImgui(app);
                cmdBuffer.endRenderingKHR();

                cmdBuffer.pipelineBarrier(
                        vk::PipelineStageFlagBits::eColorAttachmentOutput,
                        vk::PipelineStageFlagBits::eBottomOfPipe,
                        {},
                        {},
                        {},
                        vk::ImageMemoryBarrier{
                                vk::AccessFlagBits::eColorAttachmentWrite,
                                {},
                                vk::ImageLayout::eColorAttachmentOptimal,
                                vk::ImageLayout::ePresentSrcKHR,
                                {},
                                {},
                                context.swapchain.swapchain.getImages()[context.currentSwapchainImageIndex],
                                vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
                        });
            }
            cmdBuffer.end();

            vk::PipelineStageFlags waitDestStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            context.graphicsQueue.submit(vk::SubmitInfo{*presentationComplete, waitDestStageMask, *cmdBuffer, *renderComplete}, *fence);

            vk::Result presentResult = context.presentQueue.presentKHR(
                    vk::PresentInfoKHR{*renderComplete, *context.swapchain.swapchain, context.currentSwapchainImageIndex});
            check(presentResult == vk::Result::eSuccess);

            glfwPollEvents();
            bool& keepOpen = app.globalContext.get<WindowContext>().keepOpen;
            keepOpen = !glfwWindowShouldClose(context.window.windowHandle.get());
            if (!keepOpen) {
                context.device.waitIdle();
            }

            context.currentFrame = (context.currentFrame + 1) % context.commandBuffers.size();

        } catch (vk::OutOfDateKHRError const&) {
            recreatePipeline(context);
        }
    }

    void cleanup(App& app) override {
        ImGui_ImplVulkan_Shutdown();
        //    glslang::FinalizeProcess();
        //    for (auto& [_, pipeline]: app.globalCtx.at<VulkanContext>().modelPipelines) {
        //        for (auto& item: pipeline.shaders) {
        //            spvReflectDestroyShaderModule(&item.reflect);
        //        }
        //    }
    }
};
