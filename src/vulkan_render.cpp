#include "vulkan_render.hpp"

#include <fstream>
#include <filesystem>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <SPIRV/GlslangToSpv.h>

#include "math.hpp"
#include "render.hpp"
#include "shaders.hpp"
#include "geometries.hpp"

mat4f toShader(mat4 const& m) {
    return {
            std::array<float, 4>{static_cast<float>(m[0][0]), static_cast<float>(m[0][1]), static_cast<float>(m[0][2]), static_cast<float>(m[0][3])},
            std::array<float, 4>{static_cast<float>(m[1][0]), static_cast<float>(m[1][1]), static_cast<float>(m[1][2]), static_cast<float>(m[1][3])},
            std::array<float, 4>{static_cast<float>(m[2][0]), static_cast<float>(m[2][1]), static_cast<float>(m[2][2]), static_cast<float>(m[2][3])},
            std::array<float, 4>{static_cast<float>(m[3][0]), static_cast<float>(m[3][1]), static_cast<float>(m[3][2]), static_cast<float>(m[3][3])},
    };
}

mat4 calcView(Position const& eye, Look const& look) {
    vec3 center = eye + rotate(fromEuler(look), edyn::vector3_y);
    vec3 up = rotate(fromEuler(look), edyn::vector3_z);

    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);

    mat4 view{};
    view[0][0] = +s.x;
    view[1][0] = +s.y;
    view[2][0] = +s.z;
    view[0][1] = +u.x;
    view[1][1] = +u.y;
    view[2][1] = +u.z;
    view[0][2] = -f.x;
    view[1][2] = -f.y;
    view[2][2] = -f.z;
    view[3][0] = -dot(s, eye);
    view[3][1] = -dot(u, eye);
    view[3][2] = +dot(f, eye);
    view[3][3] = 1.0;
    return view;
}

mat4 calcProj(vk::Extent2D const& extent) {
    double rad = edyn::to_radians(45.0);
    double h = std::cos(0.5 * rad) / std::sin(0.5 * rad);
    double w = h * static_cast<double>(extent.height) / static_cast<double>(extent.width);
    double zNear = 0.1, zFar = 100.0;
    mat4 proj{};
    proj[0][0] = w;
    proj[1][1] = h;
    proj[2][2] = zFar / (zNear - zFar);
    proj[2][3] = -1.0;
    proj[3][2] = -(zFar * zNear) / (zFar - zNear);
    return proj;
}

mat4 getClip() {
    // vulkan clip space has inverted y and half z
    return {
            vec4{1.0, 0.0, 0.0, 0.0},
            vec4{0.0, -1.0, 0.0, 0.0},
            vec4{0.0, 0.0, 0.5, 0.0},
            vec4{0.0, 0.0, 0.5, 1.0},
    };
}

mat4 translate(mat4 m, vec3 v) {
    m[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
    return m;
}

mat4 calcModel(Position const& pos) {
    mat4 model = matrix4x4_identity;
    return translate(model, pos);
}

vk::ShaderModule createShaderModule(VulkanResource& vk, vk::ShaderStageFlagBits shaderStage, std::filesystem::path const& path) {
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    shaderFile.open(path);
    std::stringstream strStream;
    strStream << shaderFile.rdbuf();
    return vk::su::createShaderModule(*vk.device, shaderStage, strStream.str());
}

void createPipelineLayout(VulkanResource& vk) {
    auto uboAlignment = static_cast<size_t>(vk.physDev->getProperties().limits.minUniformBufferOffsetAlignment);
    vk.dynUboData.resize(uboAlignment, 16);
    vk.dynUboBuf.emplace(*vk.physDev, *vk.device, vk.dynUboData.mem_size(), vk::BufferUsageFlagBits::eUniformBuffer);
    vk.sharedUboBuf.emplace(*vk.physDev, *vk.device, sizeof(vk.sharedUboData), vk::BufferUsageFlagBits::eUniformBuffer);

    vk::DescriptorSetLayout descSetLayout = vk::su::createDescriptorSetLayout(
            *vk.device,
            {
                    {vk::DescriptorType::eUniformBuffer,        1, vk::ShaderStageFlagBits::eVertex},
                    {vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex}
            }
    );
    vk::PushConstantRange pushConstRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(mat4));
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo({}, descSetLayout, pushConstRange);
    vk.pipelineLayout = vk.device->createPipelineLayout(pipelineLayoutCreateInfo);

    vk.vertBufData.emplace(*vk.physDev, *vk.device, sizeof(coloredCubeData), vk::BufferUsageFlagBits::eVertexBuffer);
    vk::su::copyToDevice(
            *vk.device,
            vk.vertBufData->deviceMemory,
            coloredCubeData,
            sizeof(coloredCubeData) / sizeof(coloredCubeData[0])
    );
    vk.descriptorPool = vk::su::createDescriptorPool(
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

    vk::DescriptorSetAllocateInfo descSetAllocInfo(*vk.descriptorPool, descSetLayout);
    vk.descSet = vk.device->allocateDescriptorSets(descSetAllocInfo).front();

    vk::DescriptorBufferInfo sharedDescBufInfo{vk.sharedUboBuf->buffer, 0, VK_WHOLE_SIZE};
    vk::WriteDescriptorSet sharedWriteDescSet{*vk.descSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &sharedDescBufInfo};
    vk::DescriptorBufferInfo dynDescBufInfo{vk.dynUboBuf->buffer, 0, sizeof(DynamicUboData)};
    vk::WriteDescriptorSet dynWriteDescSet{*vk.descSet, 1, 0, 1, vk::DescriptorType::eUniformBufferDynamic, nullptr, &dynDescBufInfo};
    vk.device->updateDescriptorSets({sharedWriteDescSet, dynWriteDescSet}, nullptr);
}


void createPipeline(VulkanResource& vk) {
    auto [graphicsFamilyIdx, presentFamilyIdx] = vk::su::findGraphicsAndPresentQueueFamilyIndex(*vk.physDev, vk.surfData->surface);
    vk.swapChainData = vk::su::SwapChainData(
            *vk.physDev,
            *vk.device,
            vk.surfData->surface,
            vk.surfData->extent,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            {},
            graphicsFamilyIdx,
            presentFamilyIdx
    );

    glslang::InitializeProcess();
    vk::ShaderModule vertexShaderModule = createShaderModule(vk, vk::ShaderStageFlagBits::eVertex, "shaders/vertex.glsl");
    vk::ShaderModule fragmentShaderModule = createShaderModule(vk, vk::ShaderStageFlagBits::eFragment, "shaders/fragment.glsl");
    glslang::FinalizeProcess();

    vk::su::DepthBufferData depthBufData(*vk.physDev, *vk.device, vk::Format::eD16Unorm, vk.surfData->extent);

    vk.renderPass = vk::su::createRenderPass(
            *vk.device,
            vk::su::pickSurfaceFormat(vk.physDev->getSurfaceFormatsKHR(vk.surfData->surface)).format,
            depthBufData.format
    );

    vk.framebufs = vk::su::createFramebuffers(*vk.device, *vk.renderPass, vk.swapChainData->imageViews, depthBufData.imageView, vk.surfData->extent);

    if (!vk.pipelineLayout) {
        createPipelineLayout(vk);
    }

    vk.pipeline = vk::su::createGraphicsPipeline(
            *vk.device,
            *vk.pipelineCache,
            {vertexShaderModule, nullptr},
            {fragmentShaderModule, nullptr},
            sizeof(coloredCubeData[0]),
            {{vk::Format::eR32G32B32A32Sfloat, 0},
             {vk::Format::eR32G32B32A32Sfloat, 16}},
            vk::FrontFace::eClockwise,
            true,
            *vk.pipelineLayout,
            *vk.renderPass
    );
}

void recreatePipeline(VulkanResource& vk) {
    vk.device->waitIdle();
    int width, height;
    glfwGetFramebufferSize(vk.surfData->window.handle, &width, &height);
    vk.surfData->extent = vk::Extent2D(width, height);
    vk.swapChainData->clear(*vk.device);
    createPipeline(vk);
}


void setupImgui(VulkanResource& vk) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(vk.surfData->window.handle, true);
    ImGui_ImplVulkan_InitInfo init_info{
            .Instance = static_cast<VkInstance>(vk.inst),
            .PhysicalDevice = static_cast<VkPhysicalDevice>(*vk.physDev),
            .Device = static_cast<VkDevice>(*vk.device),
            .QueueFamily = vk.graphicsFamilyIdx,
            .Queue = static_cast<VkQueue>(*vk.graphicsQueue),
            .PipelineCache = static_cast<VkPipelineCache>(*vk.pipelineCache),
            .DescriptorPool = static_cast<VkDescriptorPool>(*vk.descriptorPool),
            .Subpass = 0,
            .MinImageCount = 2,
            .ImageCount = 2,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .Allocator = nullptr,
            .CheckVkResultFn = nullptr
    };
    ImGui_ImplVulkan_Init(&init_info, static_cast<VkRenderPass>(*vk.renderPass));
    std::cout << "[ImGui] " << IMGUI_VERSION << " initialized" << std::endl;

    vk.cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    ImGui_ImplVulkan_CreateFontsTexture(static_cast<VkCommandBuffer>(*vk.cmdBuf));
    vk.cmdBuf->end();
    vk.graphicsQueue->submit(vk::SubmitInfo({}, {}, *vk.cmdBuf), {});
    vk.device->waitIdle();

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void init(VulkanResource& vk) {
    std::string const appName = "Game Engine", engineName = "QEngine";
    vk.inst = vk::su::createInstance(appName, engineName, {}, vk::su::getInstanceExtensions());
    std::cout << "[Vulkan] Instance created" << std::endl;

#if !defined(NDEBUG)
    auto result = vk.inst.createDebugUtilsMessengerEXT(vk::su::makeDebugUtilsMessengerCreateInfoEXT());
    if (!result) {
        throw std::runtime_error("Failed to create debug messenger!");
    }
#endif

    std::vector<vk::PhysicalDevice> const& physDevs = vk.inst.enumeratePhysicalDevices();
    if (physDevs.empty()) {
        throw std::runtime_error("No physical vk.devices found");
    }
    vk.physDev = physDevs.front();

    vk::PhysicalDeviceProperties const& props = vk.physDev->getProperties();
    std::cout << "[Vulkan] Chose physical device: " << props.deviceName
              << std::endl;
    uint32_t apiVer = props.apiVersion;
    std::cout << "[Vulkan] Device API version: " << VK_VERSION_MAJOR(apiVer) << '.' << VK_VERSION_MINOR(apiVer) << '.' << VK_VERSION_PATCH(apiVer)
              << std::endl;

    // Creates window as well
    vk.surfData.emplace(vk.inst, "Game Engine", vk::Extent2D(640, 480));
    float xScale, yScale;
    glfwGetWindowContentScale(vk.surfData->window.handle, &xScale, &yScale);
    vk.surfData->extent = vk::Extent2D(static_cast<uint32_t>(static_cast<float>(vk.surfData->extent.width) * xScale),
                                       static_cast<uint32_t>(static_cast<float>(vk.surfData->extent.height) * yScale));

    std::tie(vk.graphicsFamilyIdx, vk.graphicsFamilyIdx) = vk::su::findGraphicsAndPresentQueueFamilyIndex(*vk.physDev, vk.surfData->surface);

    std::vector<std::string> extensions = vk::su::getDeviceExtensions();
//#if !defined(NDEBUG)
//    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//#endif
    vk.device = vk::su::createDevice(*vk.physDev, vk.graphicsFamilyIdx, extensions);

    vk::CommandPool cmdPool = vk::su::createCommandPool(*vk.device, vk.graphicsFamilyIdx);
    auto cmdBufs = vk.device->allocateCommandBuffers(vk::CommandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 16));
    vk.cmdBuf = cmdBufs[0];

    vk.graphicsQueue = vk.device->getQueue(vk.graphicsFamilyIdx, 0);
    vk.presentQueue = vk.device->getQueue(vk.presentFamilyIdx, 0);

    vk.drawFence = vk.device->createFence(vk::FenceCreateInfo());
    vk.imgAcqSem = vk.device->createSemaphore(vk::SemaphoreCreateInfo());
//    vk.imgAcqSem.push_back(vk.device->createSemaphore(vk::SemaphoreCreateInfo()));
//    vk.imgAcqSem.push_back(vk.device->createSemaphore(vk::SemaphoreCreateInfo()));

    vk.pipelineCache = vk.device->createPipelineCache(vk::PipelineCacheCreateInfo());
    createPipeline(vk);

    setupImgui(vk);
}

void renderOpaque(ExecuteContext& ctx) {
    auto& vk = ctx.resources.at<VulkanResource>();
    vk.cmdBuf->bindPipeline(vk::PipelineBindPoint::eGraphics, *vk.pipeline);
    vk.cmdBuf->bindVertexBuffers(0, vk.vertBufData->buffer, {0});
    vk.cmdBuf->setViewport(0, vk::Viewport(0.0f, 0.0f,
                                           static_cast<float>(vk.surfData->extent.width), static_cast<float>(vk.surfData->extent.height),
                                           0.0f, 1.0f));
    vk.cmdBuf->setScissor(0, vk::Rect2D({}, vk.surfData->extent));

    auto playerView = ctx.world.view<const Position, const Look, const Player>();
    for (auto [ent, pos, look]: playerView.each()) {
        SharedUboData sharedUbo{
                toShader(calcView(pos, look)),
                toShader(calcProj(vk.surfData->extent)),
                toShader(getClip())
        };
        vk::su::copyToDevice(*vk.device, vk.sharedUboBuf->deviceMemory, sharedUbo);
    }

    size_t drawIdx = 0;
    auto entView = ctx.world.view<const Position, const Orientation, const Cube>();
    for (auto [ent, pos, orien]: entView.each()) {
        vk.dynUboData[drawIdx++] = {toShader(calcModel(pos))};
//        std::cout << glm::to_string(dynUboData[drawIdx - 1].model) << std::endl;
    }
    size_t drawCount = drawIdx;

    void* devUboPtr = vk.device->mapMemory(vk.dynUboBuf->deviceMemory, 0, vk.dynUboData.mem_size());
    memcpy(devUboPtr, vk.dynUboData.data(), vk.dynUboData.mem_size());
    vk.device->unmapMemory(vk.dynUboBuf->deviceMemory);

    for (drawIdx = 0; drawIdx < drawCount; ++drawIdx) {
        uint32_t off = drawIdx * vk.dynUboData.block_size();
        vk.cmdBuf->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *vk.pipelineLayout, 0, *vk.descSet, off);
        vk.cmdBuf->draw(12 * 3, 1, 0, 0);
    }
}

void renderImGuiOverlay(ExecuteContext& ctx) {
    auto& diagnostics = ctx.resources.at<DiagnosticResource>();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    static bool open = true;
    static int corner = 0;
    if (corner != -1) {
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("Diagnostics", &open, window_flags)) {
        double avgFrameTime = diagnostics.getAvgFrameTime();
        ImGui::Text("%.3f ms/frame (%.1f FPS)", avgFrameTime / 10000000.0f, 1000000000.0f / avgFrameTime);
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Custom", nullptr, corner == -1)) corner = -1;
            if (ImGui::MenuItem("Top-left", nullptr, corner == 0)) corner = 0;
            if (ImGui::MenuItem("Top-right", nullptr, corner == 1)) corner = 1;
            if (ImGui::MenuItem("Bottom-left", nullptr, corner == 2)) corner = 2;
            if (ImGui::MenuItem("Bottom-right", nullptr, corner == 3)) corner = 3;
            if (open && ImGui::MenuItem("Close")) open = false;
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void renderImGui(ExecuteContext& ctx) {
    auto& vk = ctx.resources.at<VulkanResource>();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
//    ImGui::ShowDemoWindow();
//    auto it = world->view<UI>().each();
//    bool uiVisible = std::any_of(it.begin(), it.end(), [](std::tuple<entt::entity, UI> const& t) { return std::get<1>(t).visible; });
    renderImGuiOverlay(ctx);
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(*vk.cmdBuf));
}

void tryRenderVulkan(ExecuteContext& ctx) {
    auto pVk = ctx.resources.find<VulkanResource>();
    if (!pVk) return;

    VulkanResource& vk = *pVk;
    if (!vk.inst) {
        init(vk);
    }

    // Acquire next image and signal the semaphore
    vk::ResultValue<uint32_t> curBuf = vk.device->acquireNextImageKHR(vk.swapChainData->swapChain, vk::su::FenceTimeout, *vk.imgAcqSem, nullptr);

    if (curBuf.result == vk::Result::eSuboptimalKHR) {
        recreatePipeline(vk);
        return;
    }
    if (curBuf.result != vk::Result::eSuccess) {
        throw std::runtime_error("Invalid acquire next image KHR result");
    }
    if (vk.framebufs.size() <= curBuf.value) {
        throw std::runtime_error("Invalid framebuffer size");
    }

    vk.cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags()));
    vk::ClearValue clearColor = vk::ClearColorValue(std::array<float, 4>({{0.2f, 0.2f, 0.2f, 0.2f}}));
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
    std::array<vk::ClearValue, 2> clearVals{clearColor, clearDepth};
    vk::RenderPassBeginInfo renderPassBeginInfo(
            *vk.renderPass,
            vk.framebufs[curBuf.value],
            vk::Rect2D({}, vk.surfData->extent),
            clearVals
    );
    vk.cmdBuf->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    renderOpaque(ctx);
    renderImGui(ctx);
    vk.cmdBuf->endRenderPass();
    vk.cmdBuf->end();

    vk::PipelineStageFlags waitDestStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    // Fences need to be manually reset
    vk.device->resetFences(*vk.drawFence);
    // Wait for the image to be acquired via the semaphore, signal the drawing fence when submitted
    vk.graphicsQueue->submit(vk::SubmitInfo(*vk.imgAcqSem, waitDestStageMask, *vk.cmdBuf), *vk.drawFence);

    // Wait for the draw fence to be signaled
    while (vk::Result::eTimeout == vk.device->waitForFences(*vk.drawFence, VK_TRUE, vk::su::FenceTimeout));

    try {
        // Present frame to display
        vk::Result result = vk.presentQueue->presentKHR(vk::PresentInfoKHR({}, vk.swapChainData->swapChain, curBuf.value));
        switch (result) {
            case vk::Result::eSuccess:
            case vk::Result::eSuboptimalKHR:
                break;
            default:
                throw std::runtime_error("Bad present KHR result");
        }
    } catch (vk::OutOfDateKHRError const&) {
        recreatePipeline(vk);
    }

    glfwPollEvents();
    bool& keepOpen = ctx.resources.at<WindowResource>().keepOpen;
    keepOpen = !glfwWindowShouldClose(vk.surfData->window.handle);
    if (!keepOpen) {
        vk.device->waitIdle();
    }
}
