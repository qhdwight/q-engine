#pragma once

#include <optional>

#include <vulkan/vulkan.hpp>
#include <edyn/math/matrix3x3.hpp>
#include <backends/imgui_impl_vulkan.h>

#include "utils.hpp"
#include "state.hpp"
#include "plugin.hpp"
#include "matrix4x4.hpp"
#include "aligned_vector.hpp"

class VulkanRenderPlugin : Plugin {
public:
    void build(App& app) override;

    void execute(App& app) override;
};

struct WindowContext {
    bool isReady, keepOpen, isFocused;
};

struct mat4f {
    std::array<std::array<float, 4>, 4> col;
};

struct SharedUboData {
    mat4f view, proj, clip;
};

struct DynamicUboData {
    mat4f model;
};

struct VertexData {
    vec4 pos, color;
};

struct VulkanContext {
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
    std::optional<vk::su::BufferData> sharedUboBuf, dynUboBuf;
    std::optional<vk::DescriptorSet> descSet;
    std::optional<vk::Pipeline> pipeline;
    std::optional<vk::PipelineCache> pipelineCache;
    std::optional<vk::DescriptorPool> descriptorPool;
    std::optional<vk::Semaphore> imgAcqSem;
    std::optional<vk::Fence> drawFence;
    std::unordered_map<asset_handle_t, vk::su::BufferData> vertBufData;
    aligned_vector<DynamicUboData> dynUboData;
    SharedUboData sharedUboData;
    uint32_t graphicsFamilyIdx, presentFamilyIdx;
    ImGui_ImplVulkanH_Window imGuiWindow;
};
