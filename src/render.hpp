#pragma once

#include "shaders.hpp"
#include "utils.hpp"
#include "state.hpp"
#include "aligned_vector.hpp"

#include <vulkan/vulkan.hpp>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <optional>

struct Render {
    virtual bool isActive() = 0;

    virtual void render(world& world) = 0;
};

struct SharedUboData {
    glm::mat4x4 view, proj, clip;
};

struct DynamicUboData {
    glm::mat4x4 model;
};

struct VulkanRender : Render {
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
    std::optional<vk::su::BufferData> sharedUboBuf, dynUboBuf, vertBufData;
    std::optional<vk::DescriptorSet> descSet;
    std::optional<vk::Pipeline> pipeline;
    aligned_vector<DynamicUboData> dynUboData;
    std::vector<SharedUboData> sharedUboData;

    explicit VulkanRender(vk::Instance inInst);

    ~VulkanRender();

    void render(world& world) override;

    bool isActive() override;

    void createPipeline();

    void recreatePipeline();

    void commandBuffer(world& world, uint32_t curBufIdx);

    void createPipelineLayout();
};

std::unique_ptr<Render> getRenderEngine();
