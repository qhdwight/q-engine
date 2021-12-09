#include "geometries.hpp"
#include "shaders.hpp"
#include "utils.hpp"

#include <GLFW/glfw3.h>

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
    if (extent.width > extent.height) {
        fov *= static_cast<float>(extent.height) / static_cast<float>(extent.width);
    }

    glm::mat4x4 model = glm::mat4x4(1.0f);
    glm::mat4x4 view = glm::lookAt(glm::vec3(-5.0f, 3.0f, -10.0f),
                                   glm::vec3(0.0f, 0.0f, 0.0f),
                                   glm::vec3(0.0f, -1.0f, 0.0f));
    glm::mat4x4 projection = glm::perspective(fov, 1.0f, 0.1f, 100.0f);
    glm::mat4x4 clip = glm::mat4x4(1.0f, 0.0f, 0.0f, 0.0f,
                                   0.0f, -1.0f, 0.0f, 0.0f,
                                   0.0f, 0.0f, 0.5f, 0.0f,
                                   0.0f, 0.0f, 0.5f, 1.0f);  // vulkan clip space has inverted y and half z !
    return clip * projection * view * model;
}

int main() {
    try {
        std::string const appName = "Game Engine", engineName = "QEngine";

        initPhysics();

        vk::Instance inst = vk::su::createInstance(appName, engineName, {}, vk::su::getInstanceExtensions());

#ifndef NDEBUG
        inst.createDebugUtilsMessengerEXT(vk::su::makeDebugUtilsMessengerCreateInfoEXT());
#endif

        vk::PhysicalDevice physDev = inst.enumeratePhysicalDevices().front();

//        vk::su::WindowData window = vk::su::createWindow(appName, vk::Extent2D(500, 500));
//        vk::Win32SurfaceCreateInfoKHR{.hInstance = GetModuleHandle(NULL), .hwnd = window.handle->win32.handle};
//        inst.createWin32SurfaceKHR()
        vk::su::SurfaceData surfData(inst, appName, vk::Extent2D(500, 500));

        std::pair<uint32_t, uint32_t> graphicsAndPresentQueueFamilyIndex =
                vk::su::findGraphicsAndPresentQueueFamilyIndex(physDev, surfData.surface);
        vk::Device device = vk::su::createDevice(physDev,
                                                 graphicsAndPresentQueueFamilyIndex.first,
                                                 vk::su::getDeviceExtensions());

        vk::CommandPool cmdPool = vk::su::createCommandPool(device, graphicsAndPresentQueueFamilyIndex.first);
        vk::CommandBuffer cmdBuf = device.allocateCommandBuffers(
                        vk::CommandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1))
                .front();

        vk::Queue graphicsQueue = device.getQueue(graphicsAndPresentQueueFamilyIndex.first, 0);
        vk::Queue presentQueue = device.getQueue(graphicsAndPresentQueueFamilyIndex.second, 0);

        vk::su::SwapChainData swapChainData(physDev,
                                            device,
                                            surfData.surface,
                                            surfData.extent,
                                            vk::ImageUsageFlagBits::eColorAttachment |
                                            vk::ImageUsageFlagBits::eTransferSrc,
                                            {},
                                            graphicsAndPresentQueueFamilyIndex.first,
                                            graphicsAndPresentQueueFamilyIndex.second);

        vk::su::DepthBufferData depthBufData(physDev, device, vk::Format::eD16Unorm, surfData.extent);

        vk::su::BufferData uniformBufData(physDev, device, sizeof(glm::mat4x4),
                                          vk::BufferUsageFlagBits::eUniformBuffer);
        glm::mat4x4 mvpcMat = calcMVPCMat(surfData.extent);
        vk::su::copyToDevice(device, uniformBufData.deviceMemory, mvpcMat);

        vk::DescriptorSetLayout descSetLayout = vk::su::createDescriptorSetLayout(
                device, {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}});
        vk::PipelineLayout pipelineLayout = device.createPipelineLayout(
                vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), descSetLayout));

        vk::RenderPass renderPass = vk::su::createRenderPass(
                device,
                vk::su::pickSurfaceFormat(physDev.getSurfaceFormatsKHR(surfData.surface)).format,
                depthBufData.format);

        glslang::InitializeProcess();
        vk::ShaderModule vertexShaderModule =
                vk::su::createShaderModule(device, vk::ShaderStageFlagBits::eVertex, vertexShaderText_PC_C);
        vk::ShaderModule fragmentShaderModule =
                vk::su::createShaderModule(device, vk::ShaderStageFlagBits::eFragment, fragmentShaderText_C_C);
        glslang::FinalizeProcess();

        std::vector<vk::Framebuffer> framebufs = vk::su::createFramebuffers(
                device, renderPass, swapChainData.imageViews, depthBufData.imageView, surfData.extent);

        vk::su::BufferData vertBufData(
                physDev, device, sizeof(coloredCubeData), vk::BufferUsageFlagBits::eVertexBuffer);
        vk::su::copyToDevice(device,
                             vertBufData.deviceMemory,
                             coloredCubeData,
                             sizeof(coloredCubeData) / sizeof(coloredCubeData[0]));

        vk::DescriptorPool descPool =
                vk::su::createDescriptorPool(device, {{vk::DescriptorType::eUniformBuffer, 1}});
        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descPool, descSetLayout);
        vk::DescriptorSet descSet = device.allocateDescriptorSets(descriptorSetAllocateInfo).front();

        vk::su::updateDescriptorSets(
                device, descSet, {{vk::DescriptorType::eUniformBuffer, uniformBufData.buffer, {}}}, {});

        vk::PipelineCache pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());
        vk::Pipeline graphicsPipeline = vk::su::createGraphicsPipeline(
                device,
                pipelineCache,
                std::make_pair(vertexShaderModule, nullptr),
                std::make_pair(fragmentShaderModule, nullptr),
                sizeof(coloredCubeData[0]),
                {{vk::Format::eR32G32B32A32Sfloat, 0},
                 {vk::Format::eR32G32B32A32Sfloat, 16}},
                vk::FrontFace::eClockwise,
                true,
                pipelineLayout,
                renderPass);

        /* VULKAN_KEY_START */

        // Get the index of the next available swapchain image:
        vk::Semaphore imageAcquiredSemaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
        vk::ResultValue<uint32_t> curBuf =
                device.acquireNextImageKHR(swapChainData.swapChain, vk::su::FenceTimeout, imageAcquiredSemaphore,
                                           nullptr);
        assert(curBuf.result == vk::Result::eSuccess);
        assert(curBuf.value < framebufs.size());

        cmdBuf.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags()));

        std::array<vk::ClearValue, 2> clearVals;
        clearVals[0].color = vk::ClearColorValue(std::array<float, 4>({{0.2f, 0.2f, 0.2f, 0.2f}}));
        clearVals[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
        vk::RenderPassBeginInfo renderPassBeginInfo(renderPass,
                                                    framebufs[curBuf.value],
                                                    vk::Rect2D(vk::Offset2D(0, 0), surfData.extent),
                                                    clearVals);
        cmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
        cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
        cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descSet, nullptr);

        cmdBuf.bindVertexBuffers(0, vertBufData.buffer, {0});
        cmdBuf.setViewport(0,
                           vk::Viewport(0.0f,
                                        0.0f,
                                        static_cast<float>(surfData.extent.width),
                                        static_cast<float>(surfData.extent.height),
                                        0.0f,
                                        1.0f));
        cmdBuf.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), surfData.extent));

        cmdBuf.draw(12 * 3, 1, 0, 0);
        cmdBuf.endRenderPass();
        cmdBuf.end();

        vk::Fence drawFence = device.createFence(vk::FenceCreateInfo());

        vk::PipelineStageFlags waitDestStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo submitInfo(imageAcquiredSemaphore, waitDestStageMask, cmdBuf);
        graphicsQueue.submit(submitInfo, drawFence);

        while (vk::Result::eTimeout == device.waitForFences(drawFence, VK_TRUE, vk::su::FenceTimeout));

        vk::Result result =
                presentQueue.presentKHR(vk::PresentInfoKHR({}, swapChainData.swapChain, curBuf.value));
        switch (result) {
            case vk::Result::eSuccess:
                break;
            case vk::Result::eSuboptimalKHR:
                std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n";
                break;
            default:
                assert(false);  // an unexpected result is returned !
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        device.waitIdle();

//        while (!glfwWindowShouldClose(window.get())) {
//            glfwPollEvents();
//        }
//        glfwTerminate();
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
