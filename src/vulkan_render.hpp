#pragma once

#include <optional>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <backends/imgui_impl_vulkan.h>

#include "utils.hpp"
#include "state.hpp"
#include "aligned_vector.hpp"

struct SharedUboData {
    glm::mat4 view, proj, clip;
};

struct DynamicUboData {
    glm::mat4 model;
};

struct VulkanResource {
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
    std::optional<vk::PipelineCache> pipelineCache;
    std::optional<vk::DescriptorPool> descriptorPool;
    aligned_vector<DynamicUboData> dynUboData;
    SharedUboData sharedUboData;
    uint32_t graphicsFamilyIdx, presentFamilyIdx;
    ImGui_ImplVulkanH_Window imGuiWindow;
};

void tryRenderVulkan(World& world);
