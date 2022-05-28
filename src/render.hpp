#pragma once

#include <array>
#include <optional>

#include <spirv_reflect.h>
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

    void cleanup(App& app) override;
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

using vec2f = std::array<float, 2>;
using vec3f = std::array<float, 3>;
using vec4f = std::array<float, 4>;

struct ModelBuffers {
    vk::su::BufferData indexBufData;
    vk::su::BufferData vertBufData;
};

struct VertexAttr {
    std::string name;
    vk::Format format;
    size_t size;
    size_t offset;
};

struct Shader {
    vk::ShaderModule module;
    std::unordered_map<uint32_t, VertexAttr> vertAttrs;
    std::unordered_map<uint32_t, vk::su::BufferData> uniforms;
    size_t vertAttrStride{};
    SpvReflectShaderModule reflect{};
    uint32_t bindCount{};
    SpvReflectDescriptorBinding** bindingsReflect = nullptr;
    uint32_t inputCount{};
    SpvReflectInterfaceVariable** inputsReflect = nullptr;
};

struct Pipeline {
    std::vector<Shader> shaders;
    std::optional<vk::PipelineLayout> layout;
    std::optional<vk::Pipeline> value;
};

struct VulkanContext {
    vk::Instance inst;
    std::optional<vk::PhysicalDevice> physDev;
    std::optional<vk::Device> device;
    std::optional<vk::su::SurfaceData> surfData;
    std::optional<vk::CommandBuffer> cmdBuf;
    std::optional<vk::Queue> graphicsQueue, presentQueue;
    std::optional<vk::su::SwapChainData> swapChainData;
    std::optional<vk::RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebufs;
    std::optional<vk::DescriptorSet> descSet;
    std::optional<vk::PipelineCache> pipelineCache;
    std::optional<vk::DescriptorPool> descriptorPool;
    std::optional<vk::Semaphore> imgAcqSem;
    std::optional<vk::Fence> drawFence;
    std::unordered_map<asset_handle_t, ModelBuffers> modelBufData;
    std::unordered_map<asset_handle_t, Pipeline> modelPipelines;
    std::unordered_map<asset_handle_t, vk::su::TextureData> textures;
    aligned_vector<DynamicUboData> dynUboData;
    SharedUboData sharedUboData;
    uint32_t graphicsFamilyIdx, presentFamilyIdx;
    ImGui_ImplVulkanH_Window imGuiWindow;
};
