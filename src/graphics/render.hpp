#pragma once

#include "game_pch.hpp"

#include <spirv_reflect.h>
#include <backends/imgui_impl_vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <GLFW/glfw3.h>

#include "state.hpp"
#include "plugin.hpp"
#include "matrix4x4.hpp"
//#include "collections/aligned_vector.hpp"
//#include "utils_raii.hpp"
//#include "cubemap.hpp"

using Extensions = std::vector<const char*>;
using DeviceExtensions = std::vector<const char*>;
using Layers = std::vector<const char*>;

const std::unordered_set<std::string_view> DynamicNames{"model"sv, "material"sv};

struct Image;
struct VulkanContext;

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

//struct ModelBuffers {
//    vk::raii::su::BufferData indexBufData;
//    vk::raii::su::BufferData vertBufData;
//};

struct MemAllocator : std::shared_ptr<VmaAllocator> {
    MemAllocator() = default;

    MemAllocator(VulkanContext& context);
};

struct Image {
    MemAllocator allocator;
    vk::Image image;
    vk::raii::ImageView view = nullptr;
    VmaAllocation allocation{};

    Image(const Image&) = delete;

    Image& operator=(Image&) = delete;

    Image(Image&&) = default;

    Image& operator=(Image&&) = default;

    Image(MemAllocator const& alloc, VmaAllocationCreateInfo const& alloc_info, vk::ImageCreateInfo const& create_info);

    ~Image();
};

constexpr vk::Format DEPTH_FORMAT = vk::Format::eD32Sfloat;

struct DepthImage : Image {
    DepthImage(VulkanContext& context);

    DepthImage(DepthImage&&) = default;

    DepthImage& operator=(DepthImage&&) = default;
};

struct Window {
    Window() = default;

    Window(vk::raii::Instance const& instance, std::string_view window_name, vk::Extent2D const& extent);

    Window(const Window&) = delete;

    Window& operator=(Window&) = delete;

    Window(Window&&) = default;

    Window& operator=(Window&&) = default;

    [[nodiscard]] vk::Extent2D extent() const;

    std::string window_name;
    std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>> window_handle;
    vk::raii::SurfaceKHR surface = nullptr;
};

struct Swapchain {
    Swapchain() = default;

    Swapchain(VulkanContext& context, Swapchain&& from);

    Swapchain(const Swapchain&) = delete;

    Swapchain& operator=(Swapchain&) = delete;

    Swapchain(Swapchain&&) = default;

    Swapchain& operator=(Swapchain&&) = default;

    vk::raii::SwapchainKHR swapchain = nullptr;
    vk::SurfaceFormatKHR format;
    std::vector<vk::raii::ImageView> views;
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
//    std::map<std::pair<uint32_t, uint32_t>, vk::raii::su::BufferData> uniforms;
};

struct VulkanContext {
    vk::raii::Context ctx;
    vk::raii::Instance inst = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_util_messenger = nullptr;
    vk::raii::PhysicalDevice physical_device = nullptr;
    Window window;
    vk::raii::Device device = nullptr;
    vk::raii::Queue graphics_queue = nullptr;
    vk::raii::Queue present_queue = nullptr;
    vk::raii::CommandPool command_pool = nullptr;
    vk::raii::CommandBuffers command_buffers = nullptr;
    Swapchain swapchain;
    std::optional<DepthImage> depth_image;
    std::vector<vk::raii::Framebuffer> framebuffers;
//    std::unordered_map<asset_handle_t, vk::raii::su::TextureData> textures;
//    std::unordered_map<asset_handle_t, ModelBuffers> modelBufData;
//    std::unordered_map<asset_handle_t, CubeMapData> cubeMaps;
//    aligned_vector<Material> materialUpload;
//    aligned_vector<ModelUpload> modelUpload;
    vk::raii::RenderPass render_pass = nullptr;
    vk::raii::DescriptorPool descriptor_pool = nullptr;
    vk::raii::PipelineCache pipeline_cache = nullptr;
//    std::unordered_map<asset_handle_t, Pipeline> modelPipelines;
    vk::raii::Semaphore image_acquire_semaphore = nullptr;
    vk::raii::Fence draw_fence = nullptr;
    MemAllocator allocator;
//
//    ImGui_ImplVulkanH_Window imGuiWindow;
//    CameraUpload cameraUpload;
//    SceneUpload sceneUpload;
    uint32_t graphics_family_index, present_family_index;
};

void init(VulkanContext& vk);

void renderOpaque(App& app);

void renderImGui(App& app);

void create_swapchain(VulkanContext& vk);

void recreatePipeline(VulkanContext& vk);

void createShaderPipeline(VulkanContext& vk, Pipeline& pipeline);
