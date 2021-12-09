#include "geometries.hpp"
#include "shaders.hpp"
#include "utils.hpp"

#include <PxConfig.h>
#include <PxPhysicsAPI.h>
#include <vulkan/vulkan.hpp>
#include <SPIRV/GlslangToSpv.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <thread>
#include <optional>
#include <iostream>

template<typename T>
struct px_ptr : public std::shared_ptr<T> {
    explicit px_ptr(T *ptr) : std::shared_ptr<T>(ptr, [](T *ptr) { ptr->release(); }) {}
};

struct Vulkan {
    vk::Instance inst;
    std::optional<vk::PhysicalDevice> physDev;
    std::optional<vk::Device> device;
    std::optional<vk::su::SurfaceData> surfData;
    std::optional<vk::CommandBuffer> cmdBuf;
    std::optional<vk::Queue> graphicsQueue, presentQueue;
    std::optional<vk::su::SwapChainData> swapChainData;
    std::optional<vk::PipelineLayout> pipelineLayout;
    std::optional<vk::RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebufs;
    std::optional<vk::su::BufferData> vertBufData;
    std::optional<vk::DescriptorSet> descSet;
    std::optional<vk::Pipeline> pipeline;
};

void initPhysics() {
    physx::PxDefaultErrorCallback errorCallback;
    physx::PxDefaultAllocator allocCallback;
    px_ptr<physx::PxFoundation> foundation{
            PxCreateFoundation(PX_PHYSICS_VERSION, allocCallback, errorCallback)};
    if (!foundation) {
        throw std::runtime_error("Failed to create PhysX foundation");
    }
    px_ptr<physx::PxPhysics> physics{
            PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale(), true)};
    physx::PxSceneDesc sceneDesc(physics->getTolerancesScale());
    if (!sceneDesc.cpuDispatcher) {
        physx::PxDefaultCpuDispatcher *cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(1);
        sceneDesc.cpuDispatcher = cpuDispatcher;
    }
    if (!sceneDesc.filterShader) {
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
    }
    physx::PxScene *scene = physics->createScene(sceneDesc);
    if (!scene) {
        throw std::runtime_error("Failed to create PhysX scene");
    }
    scene->setGravity({0.0f, -9.8f, 0.0f});
    if (!physics) {
        throw std::runtime_error("Failed to create PhysX physics");
    }
}

glm::mat4x4 calcMVPCMat(vk::Extent2D const &extent) {
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

Vulkan getVulkanInstance() {
    std::string const appName = "Game Engine", engineName = "QEngine";
    return {vk::su::createInstance(appName, engineName, {}, vk::su::getInstanceExtensions())};
}

void createPipeline(Vulkan &vk) {
    std::pair<uint32_t, uint32_t> familyIdx =
            vk::su::findGraphicsAndPresentQueueFamilyIndex(*vk.physDev, vk.surfData->surface);
    vk.swapChainData = vk::su::SwapChainData(
            *vk.physDev,
            *vk.device,
            vk.surfData->surface,
            vk.surfData->extent,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            {},
            familyIdx.first,
            familyIdx.second);

    vk::su::DepthBufferData depthBufData(*vk.physDev, *vk.device, vk::Format::eD16Unorm, vk.surfData->extent);

    vk::su::BufferData uniformBufData(*vk.physDev, *vk.device, sizeof(glm::mat4x4),
                                      vk::BufferUsageFlagBits::eUniformBuffer);
    glm::mat4x4 mvpcMat = calcMVPCMat(vk.surfData->extent);
    vk::su::copyToDevice(*vk.device, uniformBufData.deviceMemory, mvpcMat);

    vk::DescriptorSetLayout descSetLayout = vk::su::createDescriptorSetLayout(
            *vk.device, {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}});
    vk.pipelineLayout = vk.device->createPipelineLayout(
            vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), descSetLayout));

    vk.renderPass = vk::su::createRenderPass(
            *vk.device,
            vk::su::pickSurfaceFormat(vk.physDev->getSurfaceFormatsKHR(vk.surfData->surface)).format,
            depthBufData.format);

    glslang::InitializeProcess();
    vk::ShaderModule vertexShaderModule =
            vk::su::createShaderModule(*vk.device, vk::ShaderStageFlagBits::eVertex, vertexShaderText_PC_C);
    vk::ShaderModule fragmentShaderModule =
            vk::su::createShaderModule(*vk.device, vk::ShaderStageFlagBits::eFragment, fragmentShaderText_C_C);
    glslang::FinalizeProcess();

    vk.framebufs = vk::su::createFramebuffers(
            *vk.device, *vk.renderPass, vk.swapChainData->imageViews, depthBufData.imageView, vk.surfData->extent);

    vk.vertBufData.emplace(*vk.physDev, *vk.device, sizeof(coloredCubeData), vk::BufferUsageFlagBits::eVertexBuffer);
    vk::su::copyToDevice(*vk.device,
                         vk.vertBufData->deviceMemory,
                         coloredCubeData,
                         sizeof(coloredCubeData) / sizeof(coloredCubeData[0]));

    vk::DescriptorPool descPool =
            vk::su::createDescriptorPool(*vk.device, {{vk::DescriptorType::eUniformBuffer, 1}});
    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descPool, descSetLayout);
    vk.descSet = vk.device->allocateDescriptorSets(descriptorSetAllocateInfo).front();

    vk::su::updateDescriptorSets(
            *vk.device, *vk.descSet, {{vk::DescriptorType::eUniformBuffer, uniformBufData.buffer, {}}}, {});

    vk.pipeline = vk::su::createGraphicsPipeline(
            *vk.device,
            vk.device->createPipelineCache(vk::PipelineCacheCreateInfo()),
            std::make_pair(vertexShaderModule, nullptr),
            std::make_pair(fragmentShaderModule, nullptr),
            sizeof(coloredCubeData[0]),
            {{vk::Format::eR32G32B32A32Sfloat, 0},
             {vk::Format::eR32G32B32A32Sfloat, 16}},
            vk::FrontFace::eClockwise,
            true,
            *vk.pipelineLayout,
            *vk.renderPass);
}

void recreatePipeline(Vulkan &vk) {
    vk.device->waitIdle();
    int width, height;
    glfwGetFramebufferSize(vk.surfData->window.handle, &width, &height);
    vk.surfData->extent = vk::Extent2D(width, height);
    vk.swapChainData->clear(*vk.device);
    vk.vertBufData->clear(*vk.device);
    createPipeline(vk);
}

void initVulkan(Vulkan &vk) {
#ifndef NDEBUG
    vk.inst.createDebugUtilsMessengerEXT(vk::su::makeDebugUtilsMessengerCreateInfoEXT());
#endif

    std::vector<vk::PhysicalDevice> const &physDevs = vk.inst.enumeratePhysicalDevices();
    if (physDevs.empty()) {
        throw std::runtime_error("No physical devices found");
    }
    vk.physDev = physDevs.front();

    vk.surfData.emplace(vk.inst, "Game Engine", vk::Extent2D(500, 500));

    std::pair<uint32_t, uint32_t> familyIdx =
            vk::su::findGraphicsAndPresentQueueFamilyIndex(*vk.physDev, vk.surfData->surface);
    vk.device = vk::su::createDevice(*vk.physDev,
                                     familyIdx.first,
                                     vk::su::getDeviceExtensions());

    vk::CommandPool cmdPool = vk::su::createCommandPool(*vk.device, familyIdx.first);
    vk.cmdBuf = vk.device->allocateCommandBuffers(
                    vk::CommandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1))
            .front();

    vk.graphicsQueue = vk.device->getQueue(familyIdx.first, 0);
    vk.presentQueue = vk.device->getQueue(familyIdx.second, 0);

    createPipeline(vk);
}

void render(Vulkan &vk) {
    vk::Semaphore imageAcquiredSemaphore = vk.device->createSemaphore(vk::SemaphoreCreateInfo());
    vk::ResultValue<uint32_t> curBuf = vk.device->acquireNextImageKHR(vk.swapChainData->swapChain, vk::su::FenceTimeout,
                                                                      imageAcquiredSemaphore, nullptr);
    assert(curBuf.result == vk::Result::eSuccess);
    assert(curBuf.value < vk.framebufs.size());

    vk.cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags()));

    std::array<vk::ClearValue, 2> clearVals;
    clearVals[0].color = vk::ClearColorValue(std::array<float, 4>({{0.2f, 0.2f, 0.2f, 0.2f}}));
    clearVals[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
    vk::RenderPassBeginInfo renderPassBeginInfo(*vk.renderPass,
                                                vk.framebufs[curBuf.value],
                                                vk::Rect2D(vk::Offset2D(0, 0), vk.surfData->extent),
                                                clearVals);
    vk.cmdBuf->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    vk.cmdBuf->bindPipeline(vk::PipelineBindPoint::eGraphics, *vk.pipeline);
    vk.cmdBuf->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *vk.pipelineLayout, 0, *vk.descSet, nullptr);

    vk.cmdBuf->bindVertexBuffers(0, vk.vertBufData->buffer, {0});
    vk.cmdBuf->setViewport(0,
                           vk::Viewport(0.0f,
                                        0.0f,
                                        static_cast<float>(vk.surfData->extent.width),
                                        static_cast<float>(vk.surfData->extent.height),
                                        0.0f,
                                        1.0f));
    vk.cmdBuf->setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk.surfData->extent));

    vk.cmdBuf->draw(12 * 3, 1, 0, 0);
    vk.cmdBuf->endRenderPass();
    vk.cmdBuf->end();

    vk::Fence drawFence = vk.device->createFence(vk::FenceCreateInfo());

    vk::PipelineStageFlags waitDestStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submitInfo(imageAcquiredSemaphore, waitDestStageMask, *vk.cmdBuf);
    vk.graphicsQueue->submit(submitInfo, drawFence);

    while (vk::Result::eTimeout == vk.device->waitForFences(drawFence, VK_TRUE, vk::su::FenceTimeout));

    try {
        vk::Result result = vk.presentQueue->presentKHR(
                vk::PresentInfoKHR({}, vk.swapChainData->swapChain, curBuf.value));
        switch (result) {
            case vk::Result::eSuccess:
                break;
            case vk::Result::eSuboptimalKHR:
                std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n";
                break;
            default:
                assert(false);  // an unexpected result is returned !
        }
    } catch (vk::OutOfDateKHRError const &outOfDateError) {
        recreatePipeline(vk);
    }
}

int main() {
    try {
        initPhysics();

        Vulkan vk = getVulkanInstance();
        initVulkan(vk);

        while (!glfwWindowShouldClose(vk.surfData->window.handle)) {
            render(vk);
            glfwPollEvents();
        }

        vk.device->waitIdle();
    }
    catch (vk::SystemError &err) {
        std::cout << "vk::SystemError: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (std::exception &err) {
        std::cerr << "std::exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
