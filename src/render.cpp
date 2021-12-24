#include "render.hpp"

#include "state.hpp"
#include "geometries.hpp"

#include <glm/gtx/string_cast.hpp>

#include <SPIRV/GlslangToSpv.h>

#include <fstream>
#include <sstream>

glm::mat4x4 calcView(position const& eye, rotation const& look) {
    return glm::lookAt(
            glm::vec3(eye),
            glm::quat(look) * glm::vec3(0.0f, 0.0f, 1.0f),
            {0.0f, -1.0f, 0.0f}
    );
}

glm::mat4x4 calcProj(vk::Extent2D const& extent) {
    float fov = glm::radians(45.0f);
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    return glm::perspective(fov, aspect, 0.1f, 100.0f);
}

glm::mat4x4 getClip() {
    return {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f, 0.0f,
            0.0f, 0.0f, 0.5f, 1.0f
    };  // vulkan clip space has inverted y and half z!
}

glm::mat4x4 calcModel(position const& pos) {
    glm::mat4x4 model(1.0f);
    return glm::translate(model, glm::vec3(pos));
}

void VulkanRender::createPipeline() {
    auto[graphicsFamilyIdx, presentFamilyIdx] = vk::su::findGraphicsAndPresentQueueFamilyIndex(*physDev, surfData->surface);
    swapChainData = vk::su::SwapChainData(
            *physDev,
            *device,
            surfData->surface,
            surfData->extent,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            {},
            graphicsFamilyIdx,
            presentFamilyIdx
    );

    glslang::InitializeProcess();
    vk::ShaderModule vertexShaderModule = createShaderModule(vk::ShaderStageFlagBits::eVertex, "shaders/vertex.glsl");
    vk::ShaderModule fragmentShaderModule = createShaderModule(vk::ShaderStageFlagBits::eFragment, "shaders/fragment.glsl");
    glslang::FinalizeProcess();

    vk::su::DepthBufferData depthBufData(*physDev, *device, vk::Format::eD16Unorm, surfData->extent);

    renderPass = vk::su::createRenderPass(
            *device,
            vk::su::pickSurfaceFormat(physDev->getSurfaceFormatsKHR(surfData->surface)).format,
            depthBufData.format
    );

    framebufs = vk::su::createFramebuffers(
            *device, *renderPass, swapChainData->imageViews, depthBufData.imageView, surfData->extent
    );

    if (!pipelineLayout) {
        createPipelineLayout();
    }

    pipeline = vk::su::createGraphicsPipeline(
            *device,
            device->createPipelineCache(vk::PipelineCacheCreateInfo()),
            {vertexShaderModule, nullptr},
            {fragmentShaderModule, nullptr},
            sizeof(coloredCubeData[0]),
            {{vk::Format::eR32G32B32A32Sfloat, 0},
             {vk::Format::eR32G32B32A32Sfloat, 16}},
            vk::FrontFace::eClockwise,
            true,
            *pipelineLayout,
            *renderPass
    );

}


void VulkanRender::createPipelineLayout() {
    dynUboData.resize(physDev->getProperties().limits.minUniformBufferOffsetAlignment, 2);
    sharedUboData.resize(2);
    dynUboBuf.emplace(*physDev, *device, dynUboData.mem_size(), vk::BufferUsageFlagBits::eUniformBuffer);
    size_t sharedSize = sharedUboData.size() * sizeof(decltype(sharedUboData)::value_type);
    sharedUboBuf.emplace(*physDev, *device, sharedSize, vk::BufferUsageFlagBits::eUniformBuffer);

    vk::DescriptorSetLayout descSetLayout = vk::su::createDescriptorSetLayout(
            *device, {
                    {vk::DescriptorType::eUniformBuffer,        1, vk::ShaderStageFlagBits::eVertex},
                    {vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex}
            }
    );
    vk::PushConstantRange pushConstRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4x4));
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo({}, descSetLayout, pushConstRange);
    pipelineLayout = device->createPipelineLayout(pipelineLayoutCreateInfo);

    vertBufData.emplace(*physDev, *device, sizeof(coloredCubeData), vk::BufferUsageFlagBits::eVertexBuffer);
    vk::su::copyToDevice(
            *device,
            vertBufData->deviceMemory,
            coloredCubeData,
            sizeof(coloredCubeData) / sizeof(coloredCubeData[0])
    );

    vk::DescriptorPool descPool = vk::su::createDescriptorPool(
            *device, {
                    {vk::DescriptorType::eUniformBuffer,        1},
                    {vk::DescriptorType::eUniformBufferDynamic, 1}
            }
    );
    vk::DescriptorSetAllocateInfo descSetAllocInfo(descPool, descSetLayout);
    descSet = device->allocateDescriptorSets(descSetAllocInfo).front();

    vk::DescriptorBufferInfo sharedDescBufInfo{sharedUboBuf->buffer, 0, VK_WHOLE_SIZE};
    vk::WriteDescriptorSet sharedWriteDescSet{*descSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &sharedDescBufInfo};
    vk::DescriptorBufferInfo dynDescBufInfo{dynUboBuf->buffer, 0, sizeof(DynamicUboData)};
    vk::WriteDescriptorSet dynWriteDescSet{*descSet, 1, 0, 1, vk::DescriptorType::eUniformBufferDynamic, nullptr, &dynDescBufInfo};
    device->updateDescriptorSets({sharedWriteDescSet, dynWriteDescSet}, nullptr);
}

void VulkanRender::recreatePipeline() {
    device->waitIdle();
    int width, height;
    glfwGetFramebufferSize(surfData->window.handle, &width, &height);
    surfData->extent = vk::Extent2D(width, height);
    swapChainData->clear(*device);
    createPipeline();
}

void VulkanRender::render(world& world) {
    vk::Semaphore imageAcquiredSemaphore = device->createSemaphore(vk::SemaphoreCreateInfo());
    vk::ResultValue<uint32_t> curBuf = device->acquireNextImageKHR(
            swapChainData->swapChain, vk::su::FenceTimeout, imageAcquiredSemaphore, nullptr
    );

    if (curBuf.result == vk::Result::eSuboptimalKHR) {
        recreatePipeline();
        return;
    }
    if (curBuf.result != vk::Result::eSuccess) {
        throw std::runtime_error("Invalid acquire next image KHR result");
    }
    if (framebufs.size() <= curBuf.value) {
        throw std::runtime_error("Invalid framebuffer size");
    }

    commandBuffer(world, curBuf.value);

    vk::Fence drawFence = device->createFence(vk::FenceCreateInfo());

    vk::PipelineStageFlags waitDestStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submitInfo(imageAcquiredSemaphore, waitDestStageMask, *cmdBuf);
    graphicsQueue->submit(submitInfo, drawFence);

    while (vk::Result::eTimeout == device->waitForFences(drawFence, VK_TRUE, vk::su::FenceTimeout));

    try {
        vk::Result result = presentQueue->presentKHR(
                vk::PresentInfoKHR({}, swapChainData->swapChain, curBuf.value));
        switch (result) {
            case vk::Result::eSuccess:
            case vk::Result::eSuboptimalKHR:
                break;
            default:
                throw std::runtime_error("Bad present KHR result");
        }
    } catch (vk::OutOfDateKHRError const& outOfDateError) {
        recreatePipeline();
    }
}

void VulkanRender::commandBuffer(world& world, uint32_t curBufIdx) {
    cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags()));

    vk::ClearValue clearColor = vk::ClearColorValue(std::array<float, 4>({{0.2f, 0.2f, 0.2f, 0.2f}}));
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
    std::array<vk::ClearValue, 2> clearVals{clearColor, clearDepth};
    vk::RenderPassBeginInfo renderPassBeginInfo(
            *renderPass,
            framebufs[curBufIdx],
            vk::Rect2D(vk::Offset2D(0, 0), surfData->extent),
            clearVals
    );

    cmdBuf->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    cmdBuf->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    cmdBuf->bindVertexBuffers(0, vertBufData->buffer, {0});
    cmdBuf->setViewport(
            0,
            vk::Viewport(
                    0.0f,
                    0.0f,
                    static_cast<float>(surfData->extent.width),
                    static_cast<float>(surfData->extent.height),
                    0.0f,
                    1.0f
            )
    );
    cmdBuf->setScissor(0, vk::Rect2D({}, surfData->extent));

    auto ts = world.reg.get<timestamp>(world.worldEnt);
    double add = std::cos(static_cast<double>(ts.ns) / 1e9);
    position eye{-5.0, 3.0, -10.0};
    SharedUboData sharedUbo{
            calcView(eye, {1.0, 0.0, 0.0, 0.0}),
            calcProj(surfData->extent),
            getClip()
    };
    vk::su::copyToDevice(*device, sharedUboBuf->deviceMemory, sharedUbo);

    size_t drawIdx = 0;
    for (auto[ent, pos, rot]: world.reg.view<const position, const rotation>().each()) {
        glm::mat4x4 model = calcModel(pos + glm::dvec3{0.0, add, 0.0});
        dynUboData[drawIdx++] = {model};
//        std::cout << glm::to_string(dynUboData[drawIdx - 1].model) << std::endl;
    }
    void* devUboPtr = device->mapMemory(dynUboBuf->deviceMemory, 0, dynUboData.mem_size());
    memcpy(devUboPtr, dynUboData.data(), dynUboData.mem_size());
    device->unmapMemory(dynUboBuf->deviceMemory);

    for (drawIdx = 0; drawIdx < dynUboData.size(); ++drawIdx) {
        uint32_t off = drawIdx * dynUboData.block_size();
        cmdBuf->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *descSet, off);
        cmdBuf->draw(12 * 3, 1, 0, 0);
    }

    cmdBuf->endRenderPass();
    cmdBuf->end();
}

bool VulkanRender::isActive() {
    return !glfwWindowShouldClose(surfData->window.handle);
}

VulkanRender::~VulkanRender() {
    device->waitIdle();
}

VulkanRender::VulkanRender(vk::Instance inInst) : inst(inInst) {
#if !defined(NDEBUG)
    inst.createDebugUtilsMessengerEXT(vk::su::makeDebugUtilsMessengerCreateInfoEXT());
#endif

    std::vector<vk::PhysicalDevice> const& physDevs = inst.enumeratePhysicalDevices();
    if (physDevs.empty()) {
        throw std::runtime_error("No physical devices found");
    }
    physDev = physDevs.front();

    surfData.emplace(inst, "Game Engine", vk::Extent2D(512, 512));
    float xScale, yScale;
    glfwGetWindowContentScale(surfData->window.handle, &xScale, &yScale);
    surfData->extent = vk::Extent2D(static_cast<uint32_t>(static_cast<float>(surfData->extent.width) * xScale),
                                    static_cast<uint32_t>(static_cast<float>(surfData->extent.height) * yScale));

    auto[graphicsFamilyIdx, presentFamilyIdx] = vk::su::findGraphicsAndPresentQueueFamilyIndex(*physDev, surfData->surface);

    std::vector<std::string> extensions = vk::su::getDeviceExtensions();
//#if !defined(NDEBUG)
//    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//#endif
    device = vk::su::createDevice(*physDev, graphicsFamilyIdx, extensions);

    vk::CommandPool cmdPool = vk::su::createCommandPool(*device, graphicsFamilyIdx);
    cmdBuf = device->allocateCommandBuffers(
            vk::CommandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1)
    ).front();

    graphicsQueue = device->getQueue(graphicsFamilyIdx, 0);
    presentQueue = device->getQueue(presentFamilyIdx, 0);

    createPipeline();
}

vk::ShaderModule VulkanRender::createShaderModule(vk::ShaderStageFlagBits shaderStage, std::filesystem::path const& path) {
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    shaderFile.open(path);
    std::stringstream strStream;
    strStream << shaderFile.rdbuf();
    return vk::su::createShaderModule(*device, shaderStage, strStream.str());
}

std::unique_ptr<Render> getRenderEngine() {
    std::string const appName = "Game Engine", engineName = "QEngine";
    return std::make_unique<VulkanRender>(vk::su::createInstance(appName, engineName, {}, vk::su::getInstanceExtensions()));
}
