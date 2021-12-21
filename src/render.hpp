#pragma once

#include "shaders.hpp"
#include "utils.hpp"

#include <vulkan/vulkan.hpp>
#include <entt/entt.hpp>

#include <optional>

struct Render {
    virtual bool isActive() = 0;

    virtual void render(entt::registry& reg) = 0;
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
    std::optional<vk::su::BufferData> vertBufData;
    std::optional<vk::DescriptorSet> descSet;
    std::optional<vk::Pipeline> pipeline;

    VulkanRender(vk::Instance inInst);

    ~VulkanRender();

    void render(entt::registry& reg);

    bool isActive();

    void createPipeline();

    void recreatePipeline();
};

std::unique_ptr<Render> getRenderEngine();
