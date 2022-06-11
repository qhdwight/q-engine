#include "render.hpp"

#include <fstream>
#include <filesystem>

#include <imgui.h>
#include <spirv_reflect.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "shaders.hpp"
#include "inspector.hpp"
#include "shader_math.hpp"

using namespace entt::literals;

#define PositionAttr "POSITION"

void createShaderModule(VulkanContext& vk, Pipeline& pipeline, vk::ShaderStageFlagBits shaderStage, std::filesystem::path const& path) {
    std::ifstream shaderFile;
    shaderFile.open(path);

    if (!shaderFile) {
        throw std::runtime_error("Failed to open shader file");
    }

    std::stringstream strStream;
    strStream << shaderFile.rdbuf();
    std::string code = strStream.str();

    EShLanguage stage = vk::su::translateShaderStage(shaderStage);

    std::array<const char*, 1> shaderStrings{code.c_str()};

    glslang::TShader shader(stage);
    shader.setStrings(shaderStrings.data(), 1);

    // Enable SPIR-V and Vulkan rules when parsing GLSL
    auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if (!shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages)) {
        puts(shader.getInfoLog());
        puts(shader.getInfoDebugLog());
        throw std::runtime_error("Failed to parse shader");
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages)) {
        puts(shader.getInfoLog());
        puts(shader.getInfoDebugLog());
        throw std::runtime_error("Failed to link shader");
    }

    std::vector<unsigned int> shaderSPV;
    glslang::GlslangToSpv(*program.getIntermediate(stage), shaderSPV);

    Shader shaderExt{vk.device->createShaderModule(vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), shaderSPV))};
    SpvReflectResult result = spvReflectCreateShaderModule(shaderSPV.size() * sizeof(unsigned int), shaderSPV.data(), &shaderExt.reflect);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to reflect shader module");
    }

    pipeline.shaders.push_back(shaderExt);
}

void createSwapChain(VulkanContext& vk) {
    auto [graphicsFamilyIdx, presentFamilyIdx] = vk::su::findGraphicsAndPresentQueueFamilyIndex(*vk.physDev, vk.surfData->surface);
    vk.swapChainData = vk::su::SwapChainData(
            *vk.physDev,
            *vk.device,
            vk.surfData->surface,
            vk.surfData->extent,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            {},
            graphicsFamilyIdx,
            presentFamilyIdx
    );
    vk::su::DepthBufferData depthBufData(*vk.physDev, *vk.device, vk::Format::eD16Unorm, vk.surfData->extent);

    vk.renderPass = vk::su::createRenderPass(
            *vk.device,
            vk::su::pickSurfaceFormat(vk.physDev->getSurfaceFormatsKHR(vk.surfData->surface)).format,
            depthBufData.format
    );

    vk.framebufs = vk::su::createFramebuffers(*vk.device, *vk.renderPass, vk.swapChainData->imageViews, depthBufData.imageView, vk.surfData->extent);
}

//bool ends_with(std::string_view value, std::string_view ending) {
//    if (ending.size() > value.size()) return false;
//    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
//}

enum class PBRWorkflows {
    MetallicRoughness = 0, SpecularGlossiness = 1
};

void createShaderPipeline(VulkanContext& vk, Pipeline& pipeline) {
    auto shadersPath = std::filesystem::current_path() / "assets" / "shaders";
    pipeline.shaders.clear();
    createShaderModule(vk, pipeline, vk::ShaderStageFlagBits::eVertex, shadersPath / "pbr.vert");
    createShaderModule(vk, pipeline, vk::ShaderStageFlagBits::eFragment, shadersPath / "pbr.frag");

    auto uboAlignment = static_cast<size_t>(vk.physDev->getProperties().limits.minUniformBufferOffsetAlignment);
    vk.modelUpload.resize(uboAlignment, 16);

    vk.materialUpload.resize(uboAlignment, 1);
    vk.materialUpload[0] = MaterialUpload{
            .baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f},
            .emissiveFactor = {0.0f, 0.0f, 0.0f, 0.0f},
            .diffuseFactor = {1.0f, 1.0f, 1.0f, 1.0f},
            .specularFactor = {0.0f, 0.0f, 0.0f, 0.0f},
            .workflow = static_cast<float>(PBRWorkflows::MetallicRoughness),
            .baseColorTextureSet = 0,
            .physicalDescriptorTextureSet = 1,
            .normalTextureSet = 2,
            .occlusionTextureSet = 3,
            .emissiveTextureSet = 4,
            .metallicFactor = 0.0f,
            .roughnessFactor = 0.2f,
            .alphaMask = 0.0f,
            .alphaMaskCutoff = 0.0f
    };

    size_t totalUniformCount = 0;
    // set -> ((stage, descType) -> count)
    std::vector<std::vector<vk::DescriptorSetLayoutBinding>> setBindings(3);
    for (Shader& shader: pipeline.shaders) {
        SpvReflectResult result;
        result = spvReflectEnumerateDescriptorBindings(&shader.reflect, &shader.bindCount, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        shader.bindingsReflect = static_cast<SpvReflectDescriptorBinding**>(malloc(shader.bindCount * sizeof(SpvReflectDescriptorBinding*)));
        result = spvReflectEnumerateDescriptorBindings(&shader.reflect, &shader.bindCount, shader.bindingsReflect);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        spvReflectEnumerateInputVariables(&shader.reflect, &shader.inputCount, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        shader.inputsReflect = static_cast<SpvReflectInterfaceVariable**>(malloc(shader.inputCount * sizeof(SpvReflectInterfaceVariable*)));
        result = spvReflectEnumerateInputVariables(&shader.reflect, &shader.inputCount, shader.inputsReflect);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        auto shaderStage = static_cast<vk::ShaderStageFlagBits>(shader.reflect.shader_stage);
//        std::vector<std::unordered_map<vk::DescriptorType, uint32_t>> setTypeCounts(3);
        for (size_t bind = 0; bind < shader.bindCount; ++bind) {
            SpvReflectDescriptorBinding* binding = shader.bindingsReflect[bind];
            auto descType = static_cast<vk::DescriptorType>(binding->descriptor_type);
            std::string_view name(binding->name);
            if (descType == vk::DescriptorType::eUniformBuffer && std::find(DynamicName.begin(), DynamicName.end(), name) != DynamicName.end()) {
                // nothing in the shader marks a uniform as dynamic, so we have to infer it from the name
                descType = vk::DescriptorType::eUniformBufferDynamic;
                binding->descriptor_type = static_cast<SpvReflectDescriptorType>(descType);
            }
            totalUniformCount++;
            auto& bindings = setBindings[binding->set];
            auto it = std::find_if(bindings.begin(), bindings.end(), [binding](vk::DescriptorSetLayoutBinding& bind) {
                return bind.binding == binding->binding;
            });
            if (it == bindings.end()) {
                bindings.emplace_back(binding->binding, descType, 1, shaderStage, nullptr);
            } else {
                it->stageFlags |= shaderStage;
            }
//            setTypeCounts[binding->set][descType]++;
        }
//        for (size_t set = 0; set < setTypeCounts.size(); ++set) {
//            for (auto& [descType, count]: setTypeCounts[set])
//                setBindings[set].push_back(vk::DescriptorSetLayoutBinding{binding->binding, descType, 1, shaderStage});
//        }
    }

    std::vector<vk::DescriptorSetLayout> descSetLayouts;
    descSetLayouts.reserve(setBindings.size());
    for (auto& bindings: setBindings)
        descSetLayouts.push_back(vk.device->createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{{}, bindings}));

//    vk::PushConstantRange pushConstRange{vk::ShaderStageFlagBits::eVertex, 0, sizeof(mat4)};
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{{}, descSetLayouts, {}};
    pipeline.layout = vk.device->createPipelineLayout(pipelineLayoutCreateInfo);

    vk::DescriptorSetAllocateInfo descSetAllocInfo(*vk.descriptorPool, descSetLayouts);
    pipeline.descSets = vk.device->allocateDescriptorSets(descSetAllocInfo);
    std::vector<vk::DescriptorBufferInfo> descBufInfos;
    std::vector<vk::WriteDescriptorSet> writeDescSets;
    std::vector<vk::CopyDescriptorSet> copyDescSets;
    std::vector<vk::DescriptorImageInfo> descImgInfos;
    descBufInfos.reserve(totalUniformCount);
    writeDescSets.reserve(totalUniformCount);
    copyDescSets.reserve(totalUniformCount);
    descImgInfos.reserve(totalUniformCount);
    for (Shader& shader: pipeline.shaders) {
        for (uint32_t bind = 0; bind < shader.bindCount; ++bind) {
            SpvReflectDescriptorBinding const* binding = shader.bindingsReflect[bind];
            auto descType = static_cast<vk::DescriptorType>(binding->descriptor_type);
            vk::DescriptorSet& descSet = pipeline.descSets[binding->set];
            std::string_view name(binding->name);
            std::pair<uint32_t, uint32_t> bindId{binding->set, binding->binding};
            switch (descType) {
                case vk::DescriptorType::eUniformBufferDynamic: {
                    vk::DeviceSize size = name == "model" ? vk.modelUpload.mem_size() : vk.materialUpload.mem_size();
                    vk::DeviceSize stride = name == "model" ? vk.modelUpload.block_size() : vk.materialUpload.block_size();
                    auto [it, _] = pipeline.uniforms.emplace(bindId, vk::su::BufferData{*vk.physDev, *vk.device, size,
                                                                                        vk::BufferUsageFlagBits::eUniformBuffer});
                    descBufInfos.emplace_back(it->second.buffer, 0, stride);
                    writeDescSets.emplace_back(descSet, binding->binding, 0, 1,
                                               vk::DescriptorType::eUniformBufferDynamic, nullptr, &descBufInfos.back());
                    break;
                }
                case vk::DescriptorType::eUniformBuffer: {
                    vk::DeviceSize size = name == "camera" ? sizeof(vk.cameraUpload) : sizeof(vk.sceneUpload);
                    auto [it, _] = pipeline.uniforms.emplace(bindId, vk::su::BufferData{*vk.physDev, *vk.device, size,
                                                                                        vk::BufferUsageFlagBits::eUniformBuffer});
                    descBufInfos.emplace_back(it->second.buffer, 0, VK_WHOLE_SIZE);
                    writeDescSets.emplace_back(descSet, binding->binding, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descBufInfos.back());
                    break;
                }
                case vk::DescriptorType::eCombinedImageSampler: {
                    switch (binding->image.dim) {
                        case SpvDim2D: {
                            vk::su::TextureData texData{*vk.physDev, *vk.device};
                            // Upload image to the GPU
                            vk.cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
                            texData.setImage(*vk.device, *vk.cmdBuf, vk::su::CheckerboardImageGenerator());
                            vk.cmdBuf->end();
                            vk.graphicsQueue->submit(vk::SubmitInfo({}, {}, *vk.cmdBuf), {});
                            vk.device->waitIdle();

                            auto [it, _] = vk.textures.emplace(binding->set + binding->binding * 1024, std::move(texData));
                            descImgInfos.emplace_back(it->second.sampler, it->second.imageData->imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
                            writeDescSets.emplace_back(descSet, binding->binding, 0,
                                                       vk::DescriptorType::eCombinedImageSampler, descImgInfos.back(), nullptr, nullptr);
                            break;
                        }
                        case SpvDimCube: {
                            CubeMapData cubeData{*vk.physDev, *vk.device};
                            vk.cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
                            cubeData.setImage(*vk.device, *vk.cmdBuf, SkyboxImageGenerator());
                            vk.cmdBuf->end();
                            vk.graphicsQueue->submit(vk::SubmitInfo({}, {}, *vk.cmdBuf), {});
                            vk.device->waitIdle();

                            auto [it, _] = vk.cubeMaps.emplace(binding->set + binding->binding * 1024, std::move(cubeData));
                            descImgInfos.emplace_back(it->second.sampler, it->second.imageData->imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
                            writeDescSets.emplace_back(descSet, binding->binding, 0,
                                                       vk::DescriptorType::eCombinedImageSampler, descImgInfos.back(), nullptr, nullptr);
                            break;
                        }
                        default: {
                            throw std::runtime_error("Unsupported image dimension type: " + std::to_string(binding->image.dim));
                        }
                    }
                    break;
                }
                default: {
                    throw std::runtime_error("Unsupported uniform descriptor type: " + vk::to_string(descType));
                }
            }
        }
    }

    vk.device->updateDescriptorSets(writeDescSets, copyDescSets);

    size_t vertexAttrOffset = 0;
    Shader& vertexShader = pipeline.shaders.front();
    std::vector<std::pair<vk::Format, uint32_t>> vertexAttrPairs;
    for (size_t layout = 0; layout < vertexShader.inputCount; ++layout) {
        auto const input = vertexShader.inputsReflect[layout];
        uint32_t compCount = input->numeric.vector.component_count;
        uint32_t elemSize = input->numeric.scalar.width; // Units are bits not bytes
        std::string name = input->name;
        if (name.find("in") == std::string::npos)
            throw std::runtime_error("Input variables must be prefixed with \"in\"");

        name = name.substr(2);
        for (auto& c: name) c = static_cast<char>(std::toupper(c));
        vk::Format format{};
        // vec3 and vec4 have identical alignment so we can treat them as same Vulkan type
        // TODO is this really okay?
        if ((compCount == 3 || compCount == 4) && elemSize == sizeof(float) * 8) {
            format = vk::Format::eR32G32B32A32Sfloat;
        } else if (compCount == 2 && elemSize == sizeof(float) * 8) {
            format = vk::Format::eR32G32Sfloat;
        }
        size_t size = sizeof(float) * compCount;

        vertexAttrPairs.emplace_back(format, vertexAttrOffset);
        auto [it, wasAdded] = vertexShader.vertAttrs.emplace(layout, VertexAttr{name, format, size, vertexAttrOffset});
        assert(wasAdded);
        vertexAttrOffset += size;
    }
    vertexShader.vertAttrStride = vertexAttrOffset;

    pipeline.value = vk::su::createGraphicsPipeline(
            *vk.device,
            *vk.pipelineCache,
            {vertexShader.module, nullptr},
            {pipeline.shaders[1].module, nullptr},
            vertexShader.vertAttrStride,
            vertexAttrPairs,
            vk::FrontFace::eCounterClockwise,
            true,
            *pipeline.layout,
            *vk.renderPass
    );
}

void recreatePipeline(VulkanContext& vk) {
    vk.device->waitIdle();
    int width, height;
    glfwGetFramebufferSize(vk.surfData->window.handle, &width, &height);
    vk.surfData->extent = vk::Extent2D(width, height);
    vk.swapChainData->clear(*vk.device);
    // Force recreation of pipelines, as they depend on the swap chain
    for (auto& [modelHandle, pipeline]: vk.modelPipelines) {
        pipeline.value.reset();
    }
    createSwapChain(vk);
}

void setupImgui(VulkanContext& vk) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(vk.surfData->window.handle, true);
    ImGui_ImplVulkan_InitInfo init_info{
            .Instance = static_cast<VkInstance>(vk.inst),
            .PhysicalDevice = static_cast<VkPhysicalDevice>(*vk.physDev),
            .Device = static_cast<VkDevice>(*vk.device),
            .QueueFamily = vk.graphicsFamilyIdx,
            .Queue = static_cast<VkQueue>(*vk.graphicsQueue),
            .PipelineCache = static_cast<VkPipelineCache>(*vk.pipelineCache),
            .DescriptorPool = static_cast<VkDescriptorPool>(*vk.descriptorPool),
            .Subpass = 0,
            .MinImageCount = 2,
            .ImageCount = 2,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .Allocator = nullptr,
            .CheckVkResultFn = nullptr
    };
    ImGui_ImplVulkan_Init(&init_info, static_cast<VkRenderPass>(*vk.renderPass));
    std::cout << "[IMGUI] " << IMGUI_VERSION << " initialized" << std::endl;

    vk.cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    ImGui_ImplVulkan_CreateFontsTexture(static_cast<VkCommandBuffer>(*vk.cmdBuf));
    vk.cmdBuf->end();
    vk.graphicsQueue->submit(vk::SubmitInfo({}, {}, *vk.cmdBuf), {});
    vk.device->waitIdle();

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void init(VulkanContext& vk) {
    std::string const appName = "Game Engine", engineName = "QEngine";
    vk.inst = vk::su::createInstance(appName, engineName, {}, vk::su::getInstanceExtensions());
    std::cout << "[Vulkan] Instance created" << std::endl;

#if !defined(NDEBUG)
    auto result = vk.inst.createDebugUtilsMessengerEXT(vk::su::makeDebugUtilsMessengerCreateInfoEXT());
    if (!result) {
        throw std::runtime_error("Failed to create debug messenger!");
    }
#endif

    std::vector<vk::PhysicalDevice> const& physDevs = vk.inst.enumeratePhysicalDevices();
    if (physDevs.empty()) {
        throw std::runtime_error("No physical vk.devices found");
    }
    vk.physDev = physDevs.front();

    vk::PhysicalDeviceProperties const& props = vk.physDev->getProperties();
    std::cout << "[Vulkan]" << " Chose physical device: " << props.deviceName
              << std::endl;
    uint32_t apiVer = props.apiVersion;
    std::cout << "[Vulkan]" << " Device API version: " << VK_VERSION_MAJOR(apiVer) << '.' << VK_VERSION_MINOR(apiVer) << '.'
              << VK_VERSION_PATCH(apiVer)
              << std::endl;

    // Creates window as well
    vk.surfData.emplace(vk.inst, "Game Engine", vk::Extent2D(640, 480));
    float xScale, yScale;
    glfwGetWindowContentScale(vk.surfData->window.handle, &xScale, &yScale);
    vk.surfData->extent = vk::Extent2D(static_cast<uint32_t>(static_cast<float>(vk.surfData->extent.width) * xScale),
                                       static_cast<uint32_t>(static_cast<float>(vk.surfData->extent.height) * yScale));

    std::tie(vk.graphicsFamilyIdx, vk.graphicsFamilyIdx) = vk::su::findGraphicsAndPresentQueueFamilyIndex(*vk.physDev, vk.surfData->surface);

    std::vector<std::string> extensions = vk::su::getDeviceExtensions();
//#if !defined(NDEBUG)
//    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//#endif
    vk.device = vk::su::createDevice(*vk.physDev, vk.graphicsFamilyIdx, extensions);

    vk::CommandPool cmdPool = vk::su::createCommandPool(*vk.device, vk.graphicsFamilyIdx);
    auto cmdBufs = vk.device->allocateCommandBuffers(vk::CommandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 16));
    vk.cmdBuf = cmdBufs[0];

    vk.graphicsQueue = vk.device->getQueue(vk.graphicsFamilyIdx, 0);
    vk.presentQueue = vk.device->getQueue(vk.presentFamilyIdx, 0);

    vk.drawFence = vk.device->createFence(vk::FenceCreateInfo());
    vk.imgAcqSem = vk.device->createSemaphore(vk::SemaphoreCreateInfo());

    vk.pipelineCache = vk.device->createPipelineCache(vk::PipelineCacheCreateInfo());

    vk.descriptorPool = vk::su::createDescriptorPool(
            *vk.device, {
                    {vk::DescriptorType::eSampler,              64},
                    {vk::DescriptorType::eCombinedImageSampler, 64},
                    {vk::DescriptorType::eSampledImage,         64},
                    {vk::DescriptorType::eStorageImage,         64},
                    {vk::DescriptorType::eUniformTexelBuffer,   64},
                    {vk::DescriptorType::eStorageTexelBuffer,   64},
                    {vk::DescriptorType::eUniformBuffer,        64},
                    {vk::DescriptorType::eStorageBuffer,        64},
                    {vk::DescriptorType::eUniformBufferDynamic, 64},
                    {vk::DescriptorType::eStorageBufferDynamic, 64},
                    {vk::DescriptorType::eInputAttachment,      64},
            }
    );

    createSwapChain(vk);

    setupImgui(vk);

    glslang::InitializeProcess();
}

template<typename T>
vk::su::BufferData createIndexBufferData(VulkanContext const& vk, entt::resource<tinygltf::Model> const& model) {
    tinygltf::Primitive& primitive = model->meshes.front().primitives.front();
    tinygltf::Accessor& acc = model->accessors.at(primitive.indices);
    assert(acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);
    tinygltf::BufferView& view = model->bufferViews.at(acc.bufferView);
    tinygltf::Buffer& buf = model->buffers.at(view.buffer);
    vk::su::BufferData bufData{*vk.physDev, *vk.device, view.byteLength, vk::BufferUsageFlagBits::eIndexBuffer};
    auto dataStart = reinterpret_cast<std::byte*>(buf.data.data());
    auto data = reinterpret_cast<T*>(dataStart + view.byteOffset + acc.byteOffset);
//    auto devData = static_cast<T*>(vk.device->mapMemory(bufData.deviceMemory, 0, view.byteLength));
    vk::su::copyToDevice(*vk.device, bufData.deviceMemory, data, acc.count);
//    for (size_t i = 0; i < acc.count; ++i) {
//        devData[i] = data[i];
//    }
//    vk.device->unmapMemory(bufData.deviceMemory);
    return bufData;
}

void tryFillAttributeBuffer(
        VulkanContext const& vk, entt::resource<tinygltf::Model> const& model, vk::su::BufferData& bufData, std::string const& attrName,
        size_t modelStride, vk::DeviceSize shaderStride, vk::DeviceSize offset
) {
    tinygltf::Primitive& primitive = model->meshes.front().primitives.front();
    // Check if this model has a corresponding attribute by name
    auto it = primitive.attributes.find(attrName);
    if (it == primitive.attributes.end()) return;

    size_t attrIdx = it->second;
    tinygltf::Accessor& acc = model->accessors.at(attrIdx);
    tinygltf::BufferView& view = model->bufferViews.at(acc.bufferView);
    tinygltf::Buffer& buf = model->buffers.at(view.buffer);
    auto modelData = reinterpret_cast<std::byte*>(buf.data.data()) + view.byteOffset + acc.byteOffset;
    auto devData = static_cast<std::byte*>(vk.device->mapMemory(bufData.deviceMemory, 0, view.byteLength)) + offset;
    for (size_t i = 0; i < acc.count; i++) {
        std::memcpy(devData, modelData, modelStride);
        modelData += modelStride;
        devData += shaderStride;
    }
    vk.device->unmapMemory(bufData.deviceMemory);
}

void renderOpaque(App& app) {
    auto& vk = app.globalCtx.at<VulkanContext>();
    vk.cmdBuf->setViewport(0, vk::Viewport(0.0f, 0.0f,
                                           static_cast<float>(vk.surfData->extent.width), static_cast<float>(vk.surfData->extent.height),
                                           0.0f, 1.0f));
    vk.cmdBuf->setScissor(0, vk::Rect2D({}, vk.surfData->extent));

    for (auto& [handle, pipeline]: vk.modelPipelines) {
        Shader const& vertShader = pipeline.shaders[0];
        vk.cmdBuf->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.value);
        auto renderCtx = app.renderWorld.ctx().at<RenderContext>();
        auto playerIt = app.renderWorld.view<const Position, const Look, const Player>().each();
        for (auto [ent, pos, look, player]: playerIt) {
            if (player.id != renderCtx.playerId) continue;

            CameraUpload camera{
                    .view = toShader(calcView(pos, look)),
                    .proj = toShader(calcProj(vk.surfData->extent)),
                    .clip = toShader(ClipMat),
                    .camPos = toShader(pos)
            };
            vk::su::copyToDevice(*vk.device, pipeline.uniforms.find({0, 0})->second.deviceMemory, camera);
        }

        SceneUpload scene{
                .lightDir = {1.0f, 1.0f, -1.0f, 0.0f},
                .exposure = 4.5f,
                .gamma = 2.2f,
                .prefilteredCubeMipLevels = 0.0f,
                .scaleIBLAmbient = 1.0f,
                .debugViewInputs = 0,
                .debugViewEquation = 0
        };

        vk::su::copyToDevice(*vk.device, pipeline.uniforms.find({0, 1})->second.deviceMemory, scene);

        size_t drawIdx;
        auto modelView = app.renderWorld.view<const Position, const Orientation, const ModelHandle>();
        // We store data per model in a dynamic UBO to save memory
        // This way we only have one upload
        drawIdx = 0;
        for (auto [ent, pos, orien, modelHandle]: modelView.each()) {
            vk.modelUpload[drawIdx++] = {toShader(calcModel(pos))};
        }

        void* mappedModelPtr = vk.device->mapMemory(pipeline.uniforms.find({2, 0})->second.deviceMemory, 0, vk.modelUpload.mem_size());
        memcpy(mappedModelPtr, vk.modelUpload.data(), vk.modelUpload.mem_size());
        vk.device->unmapMemory(pipeline.uniforms.find({2, 0})->second.deviceMemory);

        void* mappedMaterialPtr = vk.device->mapMemory(pipeline.uniforms.find({2, 1})->second.deviceMemory, 0, vk.materialUpload.mem_size());
        memcpy(mappedMaterialPtr, vk.materialUpload.data(), vk.materialUpload.mem_size());
        vk.device->unmapMemory(pipeline.uniforms.find({2, 1})->second.deviceMemory);

        // TODO: is this same order?
        drawIdx = 0;
        for (auto [ent, pos, orien, modelHandle]: modelView.each()) {
            entt::resource<Model> model;
            ModelBuffers* rawModelBuffers;
            auto modelBufIt = vk.modelBufData.find(modelHandle.value);
            // Check if we need to create a vertex buffer for this model
            if (modelBufIt == vk.modelBufData.end()) {
                auto [assetIt, wasAssetAdded] = app.modelAssets.load(modelHandle.value, "models/Cube.glb");
                assert(wasAssetAdded);
                model = assetIt->second;
                assert(model);

                size_t vertCount = model->accessors[model->meshes.front().primitives.front().attributes.at(PositionAttr)].count;
                vk::su::BufferData vertBufData{*vk.physDev, *vk.device, vertCount * vertShader.vertAttrStride,
                                               vk::BufferUsageFlagBits::eVertexBuffer};
                for (auto& [layout, attr]: vertShader.vertAttrs) {
                    tryFillAttributeBuffer(vk, model, vertBufData, attr.name, attr.size, vertShader.vertAttrStride, attr.offset);
                }
                auto [addedIt, wasBufAdded] = vk.modelBufData.emplace(modelHandle.value, ModelBuffers{
                        createIndexBufferData<uint16_t>(vk, model),
                        vertBufData
                });
                assert(wasBufAdded);
                rawModelBuffers = &addedIt->second;
            } else {
                model = app.modelAssets[modelHandle.value];
                rawModelBuffers = &modelBufIt->second;
            }
            if (rawModelBuffers) {
                vk.cmdBuf->bindVertexBuffers(0, rawModelBuffers->vertBufData.buffer, {0});
                vk.cmdBuf->bindIndexBuffer(rawModelBuffers->indexBufData.buffer, 0, vk::IndexType::eUint16);
                std::array<uint32_t, 2> dynamicOffsets{
                        static_cast<uint32_t>(drawIdx * vk.modelUpload.block_size()),
                        static_cast<uint32_t>(0 * vk.materialUpload.block_size())
                };
                vk.cmdBuf->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0u, pipeline.descSets, dynamicOffsets);
                uint32_t indexCount = model->accessors[model->meshes.front().primitives.front().indices].count;
                vk.cmdBuf->drawIndexed(indexCount, 1, 0, 0, 0);
            }
            drawIdx++;
        }
    }
}

void renderImGuiOverlay(App& app) {
    auto& diagnostics = app.globalCtx.at<DiagnosticResource>();
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    static bool open = true;
    static int corner = 0;
    if (corner != -1) {
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        windowFlags |= ImGuiWindowFlags_NoMove;
    }
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("Diagnostics", &open, windowFlags)) {
        double avgFrameTime = diagnostics.getAvgFrameTime();
        ImGui::Text("%.3f ms/frame (%.1f FPS)", avgFrameTime / 10000000.0f, 1000000000.0f / avgFrameTime);
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Custom", nullptr, corner == -1)) corner = -1;
            if (ImGui::MenuItem("Top-left", nullptr, corner == 0)) corner = 0;
            if (ImGui::MenuItem("Top-right", nullptr, corner == 1)) corner = 1;
            if (ImGui::MenuItem("Bottom-left", nullptr, corner == 2)) corner = 2;
            if (ImGui::MenuItem("Bottom-right", nullptr, corner == 3)) corner = 3;
            if (open && ImGui::MenuItem("Close")) open = false;
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void renderImGui(App& app) {
    auto& vk = app.globalCtx.at<VulkanContext>();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
//    ImGui::ShowDemoWindow();
//    auto it = world->view<UI>().each();
//    bool uiVisible = std::any_of(it.begin(), it.end(), [](std::tuple<entt::entity, UI> const& t) { return std::get<1>(t).visible; });
    renderImGuiInspector(app);
    renderImGuiOverlay(app);
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(*vk.cmdBuf));
}

void VulkanRenderPlugin::build(App& app) {
    app.globalCtx.emplace<VulkanContext>();
    app.globalCtx.emplace<WindowContext>(false, true, false);
}

void VulkanRenderPlugin::execute(App& app) {
    auto pVk = app.globalCtx.find<VulkanContext>();
    if (!pVk) return;

    VulkanContext& vk = *pVk;
    if (!vk.inst) init(vk);

    // Acquire next image and signal the semaphore
    vk::ResultValue<uint32_t> curBuf = vk.device->acquireNextImageKHR(vk.swapChainData->swapChain, vk::su::FenceTimeout, *vk.imgAcqSem, nullptr);

    if (curBuf.result == vk::Result::eSuboptimalKHR) {
        recreatePipeline(vk);
        return;
    }
    if (curBuf.result != vk::Result::eSuccess) {
        throw std::runtime_error("Invalid acquire next image KHR result");
    }
    if (vk.framebufs.size() <= curBuf.value) {
        throw std::runtime_error("Invalid framebuffer size");
    }

    for (auto [ent, shaderHandle]: app.renderWorld.view<const ShaderHandle>().each()) {
        if (vk.modelPipelines.contains(shaderHandle.value)) continue;

        // Create blank shader and fill it in
        createShaderPipeline(vk, vk.modelPipelines[shaderHandle.value]);
    }

    vk.cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags()));
    vk::ClearValue clearColor = vk::ClearColorValue(std::array<float, 4>{0.2f, 0.2f, 0.2f, 0.2f});
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
    std::array<vk::ClearValue, 2> clearVals{clearColor, clearDepth};
    vk::RenderPassBeginInfo renderPassBeginInfo(
            *vk.renderPass,
            vk.framebufs[curBuf.value],
            vk::Rect2D({}, vk.surfData->extent),
            clearVals
    );
    vk.cmdBuf->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    renderOpaque(app);
    renderImGui(app);
    vk.cmdBuf->endRenderPass();
    vk.cmdBuf->end();

    vk::PipelineStageFlags waitDestStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    // Fences need to be manually reset
    vk.device->resetFences(*vk.drawFence);
    // Wait for the image to be acquired via the semaphore, signal the drawing fence when submitted
    vk.graphicsQueue->submit(vk::SubmitInfo(*vk.imgAcqSem, waitDestStageMask, *vk.cmdBuf), *vk.drawFence);

    // Wait for the draw fence to be signaled
    while (vk::Result::eTimeout == vk.device->waitForFences(*vk.drawFence, VK_TRUE, vk::su::FenceTimeout));

    try {
        // Present frame to display
        vk::Result result = vk.presentQueue->presentKHR(vk::PresentInfoKHR({}, vk.swapChainData->swapChain, curBuf.value));
        switch (result) {
            case vk::Result::eSuccess:
            case vk::Result::eSuboptimalKHR:
                break;
            default:
                throw std::runtime_error("Bad present KHR result: " + vk::to_string(result));
        }
    } catch (vk::OutOfDateKHRError const&) {
        recreatePipeline(vk);
    }

    glfwPollEvents();
    bool& keepOpen = app.globalCtx.at<WindowContext>().keepOpen;
    keepOpen = !glfwWindowShouldClose(vk.surfData->window.handle);
    if (!keepOpen) {
        vk.device->waitIdle();
    }
}

void VulkanRenderPlugin::cleanup(App& app) {
    glslang::FinalizeProcess();
    for (auto& [_, pipeline]: app.globalCtx.at<VulkanContext>().modelPipelines) {
        for (auto& item: pipeline.shaders) {
            spvReflectDestroyShaderModule(&item.reflect);
        }
    }
}
