#pragma once

#include <array>
#include <optional>
#include <unordered_map>

#include <spirv_reflect.h>
#include <vulkan/vulkan.hpp>
#include <edyn/math/matrix3x3.hpp>
#include <backends/imgui_impl_vulkan.h>

#include "utils.hpp"
#include "../state.hpp"
#include "../plugin.hpp"
#include "cubemap.hpp"
#include "../matrix4x4.hpp"
#include "../aligned_vector.hpp"

const std::vector<std::string_view> DynamicName{"model", "material"};

using vec2f = std::array<float, 2>;
using vec3f = std::array<float, 3>;
using vec4f = std::array<float, 4>;

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
    std::unordered_map<uint32_t, VertexAttr> vertAttrs{};
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
    std::vector<vk::DescriptorSet> descSets;
    std::map<std::pair<uint32_t, uint32_t>, vk::su::BufferData> uniforms;
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
    std::optional<vk::PipelineCache> pipelineCache;
    std::optional<vk::DescriptorPool> descriptorPool;
    std::optional<vk::Semaphore> imgAcqSem;
    std::optional<vk::Fence> drawFence;
    std::unordered_map<asset_handle_t, ModelBuffers> modelBufData;
    std::unordered_map<asset_handle_t, Pipeline> modelPipelines;
    std::unordered_map<asset_handle_t, vk::su::TextureData> textures;
    std::unordered_map<asset_handle_t, CubeMapData> cubeMaps;
    aligned_vector<ModelUpload> modelUpload;
    aligned_vector<Material> materialUpload;
    CameraUpload cameraUpload;
    SceneUpload sceneUpload;
    uint32_t graphicsFamilyIdx, presentFamilyIdx;
    ImGui_ImplVulkanH_Window imGuiWindow;
};
