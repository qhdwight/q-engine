#pragma once

#include "utils.hpp"
#include "state.hpp"
#include "aligned_vector.hpp"

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <optional>

struct SharedUboData {
    glm::mat4 view, proj, clip;
};

struct DynamicUboData {
    glm::mat4 model;
};

struct VulkanData {
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
    SharedUboData sharedUboData;
};

void tryRenderVulkan(world& world);
