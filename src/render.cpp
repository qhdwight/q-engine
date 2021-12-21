#include "render.hpp"

#include "state.hpp"
#include "geometries.hpp"

#include <SPIRV/GlslangToSpv.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

glm::mat4x4 calcMVPCMat(vk::Extent2D const& extent) {
    float fov = glm::radians(45.0f);
    glm::mat4x4 model = glm::mat4x4(1.0f);
    glm::mat4x4 view = glm::lookAt(glm::vec3(-5.0f, 3.0f, -10.0f),
                                   glm::vec3(0.0f, 0.0f, 0.0f),
                                   glm::vec3(0.0f, -1.0f, 0.0f));
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    glm::mat4x4 projection = glm::perspective(fov, aspect, 0.1f, 100.0f);
    glm::mat4x4 clip = glm::mat4x4(1.0f, 0.0f, 0.0f, 0.0f,
                                   0.0f, -1.0f, 0.0f, 0.0f,
                                   0.0f, 0.0f, 0.5f, 0.0f,
                                   0.0f, 0.0f, 0.5f, 1.0f);  // vulkan clip space has inverted y and half z !
    return clip * projection * view * model;
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
            presentFamilyIdx);

    vk::su::DepthBufferData depthBufData(*physDev, *device, vk::Format::eD16Unorm, surfData->extent);

    vk::su::BufferData uniformBufData(*physDev, *device, sizeof(glm::mat4x4),
                                      vk::BufferUsageFlagBits::eUniformBuffer);
    glm::mat4x4 mvpcMat = calcMVPCMat(surfData->extent);
    vk::su::copyToDevice(*device, uniformBufData.deviceMemory, mvpcMat);

    vk::DescriptorSetLayout descSetLayout = vk::su::createDescriptorSetLayout(
            *device, {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}});
    pipelineLayout = device->createPipelineLayout(
            vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), descSetLayout));

    renderPass = vk::su::createRenderPass(
            *device,
            vk::su::pickSurfaceFormat(physDev->getSurfaceFormatsKHR(surfData->surface)).format,
            depthBufData.format);

    glslang::InitializeProcess();
    vk::ShaderModule vertexShaderModule =
            vk::su::createShaderModule(*device, vk::ShaderStageFlagBits::eVertex, vertexShaderText_PC_C);
    vk::ShaderModule fragmentShaderModule =
            vk::su::createShaderModule(*device, vk::ShaderStageFlagBits::eFragment, fragmentShaderText_C_C);
    glslang::FinalizeProcess();

    framebufs = vk::su::createFramebuffers(
            *device, *renderPass, swapChainData->imageViews, depthBufData.imageView, surfData->extent);

    vertBufData.emplace(*physDev, *device, sizeof(coloredCubeData), vk::BufferUsageFlagBits::eVertexBuffer);
    vk::su::copyToDevice(*device,
                         vertBufData->deviceMemory,
                         coloredCubeData,
                         sizeof(coloredCubeData) / sizeof(coloredCubeData[0]));

    vk::DescriptorPool descPool =
            vk::su::createDescriptorPool(*device, {{vk::DescriptorType::eUniformBuffer, 1}});
    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descPool, descSetLayout);
    descSet = device->allocateDescriptorSets(descriptorSetAllocateInfo).front();

    vk::su::updateDescriptorSets(
            *device, *descSet, {{vk::DescriptorType::eUniformBuffer, uniformBufData.buffer, {}}}, {});

    pipeline = vk::su::createGraphicsPipeline(
            *device,
            device->createPipelineCache(vk::PipelineCacheCreateInfo()),
            std::make_pair(vertexShaderModule, nullptr),
            std::make_pair(fragmentShaderModule, nullptr),
            sizeof(coloredCubeData[0]),
            {{vk::Format::eR32G32B32A32Sfloat, 0},
             {vk::Format::eR32G32B32A32Sfloat, 16}},
            vk::FrontFace::eClockwise,
            true,
            *pipelineLayout,
            *renderPass);
}

void VulkanRender::recreatePipeline() {
    device->waitIdle();
    int width, height;
    glfwGetFramebufferSize(surfData->window.handle, &width, &height);
    surfData->extent = vk::Extent2D(width, height);
    swapChainData->clear(*device);
    vertBufData->clear(*device);
    createPipeline();
}

void VulkanRender::render(entt::registry& reg) {
    auto view = reg.view<const position>();
    for (auto[ent, pos]: view.each()) {

    }
    vk::Semaphore imageAcquiredSemaphore = device->createSemaphore(vk::SemaphoreCreateInfo());
    vk::ResultValue<uint32_t> curBuf = device->acquireNextImageKHR(swapChainData->swapChain, vk::su::FenceTimeout,
                                                                   imageAcquiredSemaphore, nullptr);

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

    cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags()));

    vk::ClearValue clearColor = vk::ClearColorValue(std::array<float, 4>({{0.2f, 0.2f, 0.2f, 0.2f}}));
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
    std::array<vk::ClearValue, 2> clearVals{clearColor, clearDepth};
    vk::RenderPassBeginInfo renderPassBeginInfo(*renderPass,
                                                framebufs[curBuf.value],
                                                vk::Rect2D(vk::Offset2D(0, 0), surfData->extent),
                                                clearVals);
    cmdBuf->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    cmdBuf->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    cmdBuf->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, *descSet, nullptr);

    cmdBuf->bindVertexBuffers(0, vertBufData->buffer, {0});
    cmdBuf->setViewport(0,
                        vk::Viewport(0.0f,
                                     0.0f,
                                     static_cast<float>(surfData->extent.width),
                                     static_cast<float>(surfData->extent.height),
                                     0.0f,
                                     1.0f));
    cmdBuf->setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), surfData->extent));

    cmdBuf->draw(12 * 3, 1, 0, 0);
    cmdBuf->endRenderPass();
    cmdBuf->end();

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
                break;
            case vk::Result::eSuboptimalKHR:
                std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n";
                break;
            default:
                throw std::runtime_error("Bad present KHR result");
        }
    } catch (vk::OutOfDateKHRError const& outOfDateError) {
        recreatePipeline();
    }
}

bool VulkanRender::isActive() {
    return !glfwWindowShouldClose(surfData->window.handle);
}

VulkanRender::~VulkanRender() {
    device->waitIdle();
}

VulkanRender::VulkanRender(vk::Instance inInst) : inst(inInst) {
#ifndef NDEBUG
    inst.createDebugUtilsMessengerEXT(vk::su::makeDebugUtilsMessengerCreateInfoEXT());
#endif

    std::vector<vk::PhysicalDevice> const& physDevs = inst.enumeratePhysicalDevices();
    if (physDevs.empty()) {
        throw std::runtime_error("No physical devices found");
    }
    physDev = physDevs.front();

    surfData.emplace(inst, "Game Engine", vk::Extent2D(500, 500));

    auto[graphicsFamilyIdx, presentFamilyIdx] = vk::su::findGraphicsAndPresentQueueFamilyIndex(*physDev, surfData->surface);
    device = vk::su::createDevice(*physDev,
                                  graphicsFamilyIdx,
                                  vk::su::getDeviceExtensions());

    vk::CommandPool cmdPool = vk::su::createCommandPool(*device, graphicsFamilyIdx);
    cmdBuf = device->allocateCommandBuffers(
                    vk::CommandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1))
            .front();

    graphicsQueue = device->getQueue(graphicsFamilyIdx, 0);
    presentQueue = device->getQueue(presentFamilyIdx, 0);

    createPipeline();
}

std::unique_ptr<Render> getRenderEngine() {
    std::string const appName = "Game Engine", engineName = "QEngine";
    return std::make_unique<VulkanRender>(vk::su::createInstance(appName, engineName, {}, vk::su::getInstanceExtensions()));
}
