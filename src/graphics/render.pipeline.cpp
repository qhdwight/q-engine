#include "render.hpp"

//#include "shaders.hpp"
//#include "utils_raii.hpp"

void createShaderModule(VulkanContext& vk, Pipeline& pipeline, vk::ShaderStageFlagBits shaderStage, std::filesystem::path const& path) {
//    std::ifstream shaderFile;
//    shaderFile.open(path);
//
//    if (!shaderFile) {
//        throw std::runtime_error("Failed to open shader file");
//    }
//
//    std::stringstream strStream;
//    strStream << shaderFile.rdbuf();
//    std::string code = strStream.str();
//
//    EShLanguage stage = vk::su::translateShaderStage(shaderStage);
//
//    std::array<const char*, 1> shaderStrings{code.c_str()};
//
//    glslang::TShader shader(stage);
//    shader.setStrings(shaderStrings.data(), 1);
//
//    // Enable SPIR-V and Vulkan rules when parsing GLSL
//    auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
//
//    if (!shader.parse(GetDefaultResources(), 100, false, messages)) {
//        puts(shader.getInfoLog());
//        puts(shader.getInfoDebugLog());
//        throw std::runtime_error("Failed to parse shader");
//    }
//
//    glslang::TProgram program;
//    program.addShader(&shader);
//
//    if (!program.link(messages)) {
//        puts(shader.getInfoLog());
//        puts(shader.getInfoDebugLog());
//        throw std::runtime_error("Failed to link shader");
//    }
//
//    std::vector<unsigned int> shaderSPV;
//    glslang::GlslangToSpv(*program.getIntermediate(stage), shaderSPV);
//
//    Shader shaderExt{vk::raii::ShaderModule(*vk.device, vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), shaderSPV))};
//    SpvReflectResult result = spvReflectCreateShaderModule(shaderSPV.size() * sizeof(unsigned int), shaderSPV.data(), &shaderExt.reflect);
//    if (result != SPV_REFLECT_RESULT_SUCCESS) {
//        throw std::runtime_error("Failed to reflect shader module");
//    }
//
//    pipeline.shaders.push_back(std::move(shaderExt));
}

void createShaderPipeline(VulkanContext& vk, Pipeline& pipeline) {
//    auto shadersPath = std::filesystem::current_path() / "assets" / "shaders";
//    pipeline.shaders.clear();
//    createShaderModule(vk, pipeline, vk::ShaderStageFlagBits::eVertex, shadersPath / "pbr.vert");
//    createShaderModule(vk, pipeline, vk::ShaderStageFlagBits::eFragment, shadersPath / "pbr.frag");
//
//    auto uboAlignment = static_cast<uint32_t>(vk.physDev->getProperties().limits.minUniformBufferOffsetAlignment);
//    vk.modelUpload.resize(uboAlignment, 16);
//    vk.materialUpload.resize(uboAlignment, 16);
//
//    uint32_t totalUniformCount = 0;
//    // set -> ((stage, descType) -> count)
//    std::vector<std::vector<vk::DescriptorSetLayoutBinding>> setBindings(3);
//    for (Shader& shader: pipeline.shaders) {
//        SpvReflectResult result;
//        result = spvReflectEnumerateDescriptorBindings(&shader.reflect, &shader.bindCount, nullptr);
//        GAME_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
//        shader.bindingsReflect = static_cast<SpvReflectDescriptorBinding**>(malloc(shader.bindCount * sizeof(SpvReflectDescriptorBinding*)));
//        result = spvReflectEnumerateDescriptorBindings(&shader.reflect, &shader.bindCount, shader.bindingsReflect);
//        GAME_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
//
//        spvReflectEnumerateInputVariables(&shader.reflect, &shader.inputCount, nullptr);
//        GAME_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
//        shader.inputsReflect = static_cast<SpvReflectInterfaceVariable**>(malloc(shader.inputCount * sizeof(SpvReflectInterfaceVariable*)));
//        result = spvReflectEnumerateInputVariables(&shader.reflect, &shader.inputCount, shader.inputsReflect);
//        GAME_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
//
//        auto shaderStage = static_cast<vk::ShaderStageFlagBits>(shader.reflect.shader_stage);
////        std::vector<std::unordered_map<vk::DescriptorType, uint32_t>> setTypeCounts(3);
//        for (uint32_t bind = 0; bind < shader.bindCount; ++bind) {
//            SpvReflectDescriptorBinding* binding = shader.bindingsReflect[bind];
//            auto descType = static_cast<vk::DescriptorType>(binding->descriptor_type);
//            std::string_view name(binding->name);
//            if (descType == vk::DescriptorType::eUniformBuffer && DynamicNames.contains(name)) {
//                // nothing in the shader marks a uniform as dynamic, so we have to infer it from the name
//                descType = vk::DescriptorType::eUniformBufferDynamic;
//                binding->descriptor_type = static_cast<SpvReflectDescriptorType>(descType);
//            }
//            totalUniformCount++;
//            auto& bindings = setBindings[binding->set];
//            auto it = std::find_if(bindings.begin(), bindings.end(), [binding](vk::DescriptorSetLayoutBinding& bind) {
//                return bind.binding == binding->binding;
//            });
//            if (it == bindings.end()) {
//                bindings.emplace_back(binding->binding, descType, 1, shaderStage, nullptr);
//            } else {
//                it->stageFlags |= shaderStage;
//            }
////            setTypeCounts[binding->set][descType]++;
//        }
////        for (size_t set = 0; set < setTypeCounts.size(); ++set) {
////            for (auto& [descType, count]: setTypeCounts[set])
////                setBindings[set].push_back(vk::DescriptorSetLayoutBinding{binding->binding, descType, 1, shaderStage});
////        }
//    }
//
//    pipeline.descSetLayouts.clear();
//    pipeline.descSetLayouts.reserve(setBindings.size());
//    for (auto& bindings: setBindings)
//        pipeline.descSetLayouts.emplace_back(*vk.device, vk::DescriptorSetLayoutCreateInfo{{}, bindings});
//
//    std::vector<vk::DescriptorSetLayout> proxyDescSetLayouts;
//    proxyDescSetLayouts.reserve(setBindings.size());
//    for (auto& descSetLayout: pipeline.descSetLayouts) proxyDescSetLayouts.push_back(*descSetLayout);
//
////    vk::PushConstantRange pushConstRange{vk::ShaderStageFlagBits::eVertex, 0, sizeof(mat4)};
//    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{{}, proxyDescSetLayouts, {}};
//    pipeline.layout = vk::raii::PipelineLayout{*vk.device, pipelineLayoutCreateInfo};
//
//    pipeline.descSets = vk::raii::DescriptorSets{*vk.device, {**vk.descriptorPool, proxyDescSetLayouts}};
//    std::vector<vk::DescriptorBufferInfo> descBufInfos;
//    std::vector<vk::WriteDescriptorSet> writeDescSets;
//    std::vector<vk::CopyDescriptorSet> copyDescSets;
//    std::vector<vk::DescriptorImageInfo> descImgInfos;
//    descBufInfos.reserve(totalUniformCount);
//    writeDescSets.reserve(totalUniformCount);
//    copyDescSets.reserve(totalUniformCount);
//    descImgInfos.reserve(totalUniformCount);
//    for (Shader& shader: pipeline.shaders) {
//        for (uint32_t bind = 0; bind < shader.bindCount; ++bind) {
//            SpvReflectDescriptorBinding const* binding = shader.bindingsReflect[bind];
//            auto descType = static_cast<vk::DescriptorType>(binding->descriptor_type);
//            vk::raii::DescriptorSet& descSet = pipeline.descSets[binding->set];
//            std::string_view name(binding->name);
//            std::pair<uint32_t, uint32_t> bindId{binding->set, binding->binding};
//            switch (descType) {
//                case vk::DescriptorType::eUniformBufferDynamic: {
//                    vk::DeviceSize size = name == "model" ? vk.modelUpload.mem_size() : vk.materialUpload.mem_size();
//                    vk::DeviceSize stride = name == "model" ? vk.modelUpload.block_size() : vk.materialUpload.block_size();
//                    auto [it, _] = pipeline.uniforms.emplace(bindId, vk::raii::su::BufferData{*vk.physDev, *vk.device, size,
//                                                                                              vk::BufferUsageFlagBits::eUniformBuffer});
//                    descBufInfos.emplace_back(**it->second.buffer, 0, stride);
//                    writeDescSets.emplace_back(*descSet, binding->binding, 0, 1,
//                                               vk::DescriptorType::eUniformBufferDynamic, nullptr, &descBufInfos.back());
//                    break;
//                }
//                case vk::DescriptorType::eUniformBuffer: {
//                    vk::DeviceSize size = name == "camera" ? sizeof(vk.cameraUpload) : sizeof(vk.sceneUpload);
//                    auto [it, _] = pipeline.uniforms.emplace(bindId, vk::raii::su::BufferData{*vk.physDev, *vk.device, size,
//                                                                                              vk::BufferUsageFlagBits::eUniformBuffer});
//                    descBufInfos.emplace_back(**it->second.buffer, 0, VK_WHOLE_SIZE);
//                    writeDescSets.emplace_back(*descSet, binding->binding, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descBufInfos.back());
//                    break;
//                }
//                case vk::DescriptorType::eCombinedImageSampler: {
//                    switch (binding->image.dim) {
//                        case SpvDim2D: {
//                            vk::raii::su::TextureData texData{*vk.physDev, *vk.device};
//                            // Upload image to the GPU
//                            vk::raii::su::oneTimeSubmit(vk.cmdBufs->front(), *vk.graphicsQueue,
//                                                        [&texData](vk::raii::CommandBuffer const& cmdBuf) {
//                                                            texData.setImage(cmdBuf, vk::su::MonochromeImageGenerator({255, 255, 255}));
//                                                        });
//
//                            auto [it, _] = vk.textures.emplace(binding->set + binding->binding * 1024, std::move(texData));
//                            descImgInfos.emplace_back(*it->second.sampler, **it->second.imageData->imageView,
//                                                      vk::ImageLayout::eShaderReadOnlyOptimal);
//                            writeDescSets.emplace_back(*descSet, binding->binding, 0,
//                                                       vk::DescriptorType::eCombinedImageSampler, descImgInfos.back(), nullptr, nullptr);
//                            break;
//                        }
//                        case SpvDimCube: {
//                            CubeMapData cubeData{*vk.physDev, *vk.device};
//                            vk::raii::su::oneTimeSubmit(vk.cmdBufs->front(), *vk.graphicsQueue,
//                                                        [&cubeData, &vk](vk::raii::CommandBuffer const& cmdBuf) {
//                                                            cubeData.setImage(*vk.device, cmdBuf, SkyboxImageGenerator());
//                                                        });
//
//                            auto [it, _] = vk.cubeMaps.emplace(binding->set + binding->binding * 1024, std::move(cubeData));
//                            descImgInfos.emplace_back(*it->second.sampler, **it->second.imageData->imageView,
//                                                      vk::ImageLayout::eShaderReadOnlyOptimal);
//                            writeDescSets.emplace_back(*descSet, binding->binding, 0,
//                                                       vk::DescriptorType::eCombinedImageSampler, descImgInfos.back(), nullptr, nullptr);
//                            break;
//                        }
//                        default: {
//                            throw std::runtime_error("Unsupported image dimension type: " + std::to_string(binding->image.dim));
//                        }
//                    }
//                    break;
//                }
//                default: {
//                    throw std::runtime_error("Unsupported uniform descriptor type: " + vk::to_string(descType));
//                }
//            }
//        }
//    }
//
//    vk.device->updateDescriptorSets(writeDescSets, copyDescSets);
//
//    uint32_t vertexAttrOffset = 0;
//    Shader& vertexShader = pipeline.shaders.front();
//    std::vector<std::pair<vk::Format, uint32_t>> vertexAttrPairs;
//    for (uint32_t layout = 0; layout < vertexShader.inputCount; ++layout) {
//        auto const input = vertexShader.inputsReflect[layout];
//        uint32_t compCount = input->numeric.vector.component_count;
//        uint32_t elemSize = input->numeric.scalar.width; // Units are bits not bytes
//        std::string name = input->name;
//        if (name.find("in") == std::string::npos)
//            throw std::runtime_error("Input variables must be prefixed with \"in\"");
//
//        name = name.substr(2);
//        for (auto& c: name) c = static_cast<char>(std::toupper(c));
//        vk::Format format{};
//        // vec3 and vec4 have identical alignment so we can treat them as same Vulkan type
//        // TODO is this really okay?
//        if ((compCount == 3 || compCount == 4) && elemSize == sizeof(float) * 8) {
//            format = vk::Format::eR32G32B32A32Sfloat;
//        } else if (compCount == 2 && elemSize == sizeof(float) * 8) {
//            format = vk::Format::eR32G32Sfloat;
//        }
//        uint32_t size = sizeof(float) * compCount;
//
//        vertexAttrPairs.emplace_back(format, vertexAttrOffset);
//        auto [it, wasAdded] = vertexShader.vertAttrs.emplace(layout, VertexAttr{name, format, size, vertexAttrOffset});
//        GAME_ASSERT(wasAdded);
//        vertexAttrOffset += size;
//    }
//    vertexShader.vertAttrStride = vertexAttrOffset;
//
//    pipeline.value = vk::raii::su::makeGraphicsPipeline(
//            *vk.device,
//            *vk.pipelineCache,
//            vertexShader.module,
//            nullptr,
//            pipeline.shaders[1].module,
//            nullptr,
//            vertexShader.vertAttrStride,
//            vertexAttrPairs,
//            vk::FrontFace::eCounterClockwise,
//            true,
//            *pipeline.layout,
//            *vk.renderPass
//    );
}

void recreatePipeline(VulkanContext& vk) {
//    vk.device->waitIdle();
//    int width, height;
//    glfwGetFramebufferSize(vk.surfData->window.handle, &width, &height);
//    vk.surfData->extent = vk::Extent2D(width, height);
//    // Force recreation of pipelines, as they depend on the swap chain
//    vk.modelPipelines.clear();
//    createSwapChain(vk);
}
