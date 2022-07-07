#pragma once

#include "game_pch.hpp"

#include <spirv_reflect.h>
#include <vulkan/vulkan_raii.hpp>
#include <backends/imgui_impl_vulkan.h>

#include "state.hpp"
#include "plugin.hpp"
#include "matrix4x4.hpp"
#include "collections/aligned_vector.hpp"
#include "utils_raii.hpp"
#include "cubemap.hpp"

const std::unordered_set<std::string_view> DynamicNames{"model", "material"};

using vec2f = std::array<float, 2>;
using vec3f = std::array<float, 3>;
using vec4f = std::array<float, 4>;

class VulkanRenderPlugin : public Plugin {
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

enum class PBRWorkflows {
    MetallicRoughness = 0, SpecularGlossiness = 1
};

struct CameraUpload {
    mat4f view, proj, clip;
    vec3f camPos;
};

struct ModelUpload {
    mat4f matrix;
};

struct SceneUpload {
    vec4f lightDir;
    float exposure;
    float gamma;
    float prefilteredCubeMipLevels;
    float scaleIBLAmbient;
    float debugViewInputs;
    float debugViewEquation;
};

struct Material {
    vec4f baseColorFactor;
    vec4f emissiveFactor;
    vec4f diffuseFactor;
    vec4f specularFactor;
    float workflow;
    int baseColorTextureSet;
    int physicalDescriptorTextureSet;
    int normalTextureSet;
    int occlusionTextureSet;
    int emissiveTextureSet;
    float metallicFactor;
    float roughnessFactor;
    float alphaMask;
    float alphaMaskCutoff;
};

struct ModelBuffers {
    vk::raii::su::BufferData indexBufData;
    vk::raii::su::BufferData vertBufData;
};

struct VertexAttr {
    std::string name;
    vk::Format format;
    uint32_t size;
    uint32_t offset;
};

struct Shader {
    vk::raii::ShaderModule module;
    std::unordered_map<uint32_t, VertexAttr> vertAttrs{};
    uint32_t vertAttrStride{};
    SpvReflectShaderModule reflect{};
    uint32_t bindCount{};
    SpvReflectDescriptorBinding** bindingsReflect = nullptr;
    uint32_t inputCount{};
    SpvReflectInterfaceVariable** inputsReflect = nullptr;
};

struct Pipeline {
    std::vector<Shader> shaders;
    std::optional<vk::raii::Pipeline> value;
    std::optional<vk::raii::PipelineLayout> layout;
    std::vector<vk::raii::DescriptorSetLayout> descSetLayouts;
    std::vector<vk::raii::DescriptorSet> descSets;
    std::map<std::pair<uint32_t, uint32_t>, vk::raii::su::BufferData> uniforms;
};

struct VulkanContext {
    vk::raii::Context ctx;
    std::optional<vk::raii::Instance> inst;
    std::optional<vk::raii::DebugUtilsMessengerEXT> debugUtilMessenger;
    std::optional<vk::raii::PhysicalDevice> physDev;
    std::optional<vk::raii::su::SurfaceData> surfData;
    std::optional<vk::raii::Device> device;
    std::optional<vk::raii::Queue> graphicsQueue, presentQueue;
    std::optional<vk::raii::CommandPool> cmdPool;
    std::optional<vk::raii::CommandBuffers> cmdBufs;
    std::optional<vk::raii::su::SwapChainData> swapChainData;
    std::optional<vk::raii::su::DepthBufferData> depthBufferData;
    std::vector<vk::raii::Framebuffer> framebufs;
    std::unordered_map<asset_handle_t, vk::raii::su::TextureData> textures;
    std::unordered_map<asset_handle_t, ModelBuffers> modelBufData;
    std::unordered_map<asset_handle_t, CubeMapData> cubeMaps;
    aligned_vector<Material> materialUpload;
    aligned_vector<ModelUpload> modelUpload;
    std::optional<vk::raii::RenderPass> renderPass;
    std::optional<vk::raii::DescriptorPool> descriptorPool;
    std::optional<vk::raii::PipelineCache> pipelineCache;
    std::unordered_map<asset_handle_t, Pipeline> modelPipelines;
    std::optional<vk::raii::Semaphore> imgAcqSem;
    std::optional<vk::raii::Fence> drawFence;

    ImGui_ImplVulkanH_Window imGuiWindow;
    CameraUpload cameraUpload;
    SceneUpload sceneUpload;
    uint32_t graphicsFamilyIdx, presentFamilyIdx;
};

void renderOpaque(App& app);

void renderImGui(App& app);

void createSwapChain(VulkanContext& vk);

void recreatePipeline(VulkanContext& vk);

void createShaderPipeline(VulkanContext& vk, Pipeline& pipeline);
