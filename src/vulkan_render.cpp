#include "vulkan_render.hpp"

#include <fstream>
#include <filesystem>

#include <SPIRV/GlslangToSpv.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_access.hpp>

#include "math.hpp"
#include "render.hpp"
#include "shaders.hpp"
#include "geometries.hpp"

glm::dmat4 calcView(position const& eye, look const& look) {
    glm::dvec3 right{1.0, 0.0, 0.0}, fwd{0.0, 1.0, 0.0}, up{0.0, 0.0, 1.0};
    return glm::lookAtRH(eye.vec, eye.vec + fromEuler(look.vec) * fwd, fromEuler(look.vec) * up);
}

glm::dmat4 calcProj(vk::Extent2D const& extent) {
    return glm::perspectiveFovRH_ZO(
            glm::radians(45.0),
            static_cast<double>(extent.width), static_cast<double>(extent.height),
            0.1, 100.0
    );
}

glm::dmat4 getClip() {
    return { // vulkan clip space has inverted y and half z!
            1.0, 0.0, 0.0, 0.0,
            0.0, -1.0, 0.0, 0.0,
            0.0, 0.0, 0.5, 0.0,
            0.0, 0.0, 0.5, 1.0
    };
}

glm::dmat4 calcModel(position const& pos) {
    glm::dmat4 model(1.0);
    return glm::translate(model, pos.vec);
}

vk::ShaderModule createShaderModule(VulkanData& vk, vk::ShaderStageFlagBits shaderStage, std::filesystem::path const& path) {
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    shaderFile.open(path);
    std::stringstream strStream;
    strStream << shaderFile.rdbuf();
    return vk::su::createShaderModule(*vk.device, shaderStage, strStream.str());
}

void createPipelineLayout(VulkanData& vk) {
    size_t uboAlignment = vk.physDev->getProperties().limits.minUniformBufferOffsetAlignment;
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
    vk::PushConstantRange pushConstRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4));
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo({}, descSetLayout, pushConstRange);
    vk.pipelineLayout = vk.device->createPipelineLayout(pipelineLayoutCreateInfo);

    vk.vertBufData.emplace(*vk.physDev, *vk.device, sizeof(coloredCubeData), vk::BufferUsageFlagBits::eVertexBuffer);
    vk::su::copyToDevice(
            *vk.device,
            vk.vertBufData->deviceMemory,
            coloredCubeData,
            sizeof(coloredCubeData) / sizeof(coloredCubeData[0])
    );

    vk::DescriptorPool descPool = vk::su::createDescriptorPool(
            *vk.device, {
                    {vk::DescriptorType::eUniformBuffer,        1},
                    {vk::DescriptorType::eUniformBufferDynamic, 1}
            }
    );
    vk::DescriptorSetAllocateInfo descSetAllocInfo(descPool, descSetLayout);
    vk.descSet = vk.device->allocateDescriptorSets(descSetAllocInfo).front();

    vk::DescriptorBufferInfo sharedDescBufInfo{vk.sharedUboBuf->buffer, 0, VK_WHOLE_SIZE};
    vk::WriteDescriptorSet sharedWriteDescSet{*vk.descSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &sharedDescBufInfo};
    vk::DescriptorBufferInfo dynDescBufInfo{vk.dynUboBuf->buffer, 0, sizeof(DynamicUboData)};
    vk::WriteDescriptorSet dynWriteDescSet{*vk.descSet, 1, 0, 1, vk::DescriptorType::eUniformBufferDynamic, nullptr, &dynDescBufInfo};
    vk.device->updateDescriptorSets({sharedWriteDescSet, dynWriteDescSet}, nullptr);
}


void createPipeline(VulkanData& vk) {
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

    vk.framebufs = vk::su::createFramebuffers(
            *vk.device, *vk.renderPass, vk.swapChainData->imageViews, depthBufData.imageView, vk.surfData->extent
    );

    if (!vk.pipelineLayout) {
        createPipelineLayout(vk);
    }

    vk.pipeline = vk::su::createGraphicsPipeline(
            *vk.device,
            vk.device->createPipelineCache(vk::PipelineCacheCreateInfo()),
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

void recreatePipeline(VulkanData& vk) {
    vk.device->waitIdle();
    int width, height;
    glfwGetFramebufferSize(vk.surfData->window.handle, &width, &height);
    vk.surfData->extent = vk::Extent2D(width, height);
    vk.swapChainData->clear(*vk.device);
    createPipeline(vk);
}

void commandBuffer(VulkanData& vk, World& world, uint32_t curBufIdx) {
    vk.cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags()));

    vk::ClearValue clearColor = vk::ClearColorValue(std::array<float, 4>({{0.2f, 0.2f, 0.2f, 0.2f}}));
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
    std::array<vk::ClearValue, 2> clearVals{clearColor, clearDepth};
    vk::RenderPassBeginInfo renderPassBeginInfo(
            *vk.renderPass,
            vk.framebufs[curBufIdx],
            vk::Rect2D(vk::Offset2D(0, 0), vk.surfData->extent),
            clearVals
    );

    vk.cmdBuf->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    vk.cmdBuf->bindPipeline(vk::PipelineBindPoint::eGraphics, *vk.pipeline);
    vk.cmdBuf->bindVertexBuffers(0, vk.vertBufData->buffer, {0});
    vk.cmdBuf->setViewport(
            0,
            vk::Viewport(
                    0.0f,
                    0.0f,
                    static_cast<float>(vk.surfData->extent.width),
                    static_cast<float>(vk.surfData->extent.height),
                    0.0f,
                    1.0f
            )
    );
    vk.cmdBuf->setScissor(0, vk::Rect2D({}, vk.surfData->extent));

    auto playerView = world.reg.view<const position, const look, const Player>();
    for (auto [ent, pos, look]: playerView.each()) {
        SharedUboData sharedUbo{
                calcView(pos, look),
                calcProj(vk.surfData->extent),
                getClip()
        };
        vk::su::copyToDevice(*vk.device, vk.sharedUboBuf->deviceMemory, sharedUbo);
    }

    size_t drawIdx = 0;
    auto entView = world.reg.view<const position, const orientation, const Cube>();
    for (auto [ent, pos, orien]: entView.each()) {
        vk.dynUboData[drawIdx++] = {calcModel(pos)};
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

    vk.cmdBuf->endRenderPass();
    vk.cmdBuf->end();
}

void init(VulkanData& vk) {
    std::string const appName = "Game Engine", engineName = "QEngine";
    vk.inst = vk::su::createInstance(appName, engineName, {}, vk::su::getInstanceExtensions());

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

    vk.surfData.emplace(vk.inst, "Game Engine", vk::Extent2D(512, 512));
    float xScale, yScale;
    glfwGetWindowContentScale(vk.surfData->window.handle, &xScale, &yScale);
    vk.surfData->extent = vk::Extent2D(static_cast<uint32_t>(static_cast<float>(vk.surfData->extent.width) * xScale),
                                       static_cast<uint32_t>(static_cast<float>(vk.surfData->extent.height) * yScale));

    auto [graphicsFamilyIdx, presentFamilyIdx] = vk::su::findGraphicsAndPresentQueueFamilyIndex(*vk.physDev, vk.surfData->surface);

    std::vector<std::string> extensions = vk::su::getDeviceExtensions();
//#if !defined(NDEBUG)
//    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//#endif
    vk.device = vk::su::createDevice(*vk.physDev, graphicsFamilyIdx, extensions);

    vk::CommandPool cmdPool = vk::su::createCommandPool(*vk.device, graphicsFamilyIdx);
    vk.cmdBuf = vk.device->allocateCommandBuffers(vk::CommandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1)).front();

    vk.graphicsQueue = vk.device->getQueue(graphicsFamilyIdx, 0);
    vk.presentQueue = vk.device->getQueue(presentFamilyIdx, 0);

    createPipeline(vk);
}

void tryRenderVulkan(World& world) {
    auto pVk = world.reg.try_get<VulkanData>(world.sharedEnt);
    if (!pVk) return;

    VulkanData& vk = *pVk;
    if (!vk.inst) {
        init(vk);
    }

    vk::Semaphore imgAcqSem = vk.device->createSemaphore(vk::SemaphoreCreateInfo());
    vk::ResultValue<uint32_t> curBuf = vk.device->acquireNextImageKHR(vk.swapChainData->swapChain, vk::su::FenceTimeout, imgAcqSem, nullptr);

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

    commandBuffer(vk, world, curBuf.value);

    vk::Fence drawFence = vk.device->createFence(vk::FenceCreateInfo());

    vk::PipelineStageFlags waitDestStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submitInfo(imgAcqSem, waitDestStageMask, *vk.cmdBuf);
    vk.graphicsQueue->submit(submitInfo, drawFence);

    while (vk::Result::eTimeout == vk.device->waitForFences(drawFence, VK_TRUE, vk::su::FenceTimeout));

    try {
        vk::Result result = vk.presentQueue->presentKHR(vk::PresentInfoKHR({}, vk.swapChainData->swapChain, curBuf.value));
        switch (result) {
            case vk::Result::eSuccess:
            case vk::Result::eSuboptimalKHR:
                break;
            default:
                throw std::runtime_error("Bad present KHR result");
        }
    } catch (vk::OutOfDateKHRError const& outOfDateError) {
        recreatePipeline(vk);
    }

    glfwPollEvents();
    bool& keepOpen = world.reg.get<Window>(world.sharedEnt).keepOpen;
    keepOpen = !glfwWindowShouldClose(vk.surfData->window.handle);
    if (!keepOpen) {
        vk.device->waitIdle();
    }
}
