#include "render.hpp"

#include "app.hpp"
#include "inspector.hpp"
#include "shader_math.hpp"

#define PositionAttr "POSITION"

//template<std::integral T>
//vk::raii::su::BufferData createIndexBufferData(VulkanContext const& vk, entt::resource<tinygltf::Model> const& model) {
//    tinygltf::Primitive& primitive = model->meshes.front().primitives.front();
//    tinygltf::Accessor& acc = model->accessors.at(primitive.indices);
//    GAME_ASSERT(acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);
//    tinygltf::BufferView& view = model->bufferViews.at(acc.bufferView);
//    tinygltf::Buffer& buf = model->buffers.at(view.buffer);
//    vk::raii::su::BufferData bufData{*vk.physDev, *vk.device, view.byteLength, vk::BufferUsageFlagBits::eIndexBuffer};
//    auto dataStart = reinterpret_cast<std::byte*>(buf.data.data());
//    auto data = reinterpret_cast<T*>(dataStart + view.byteOffset + acc.byteOffset);
////    auto devData = static_cast<T*>(vk.device->mapMemory(bufData.deviceMemory, 0, view.byteLength));
//    vk::raii::su::copyToDevice(*bufData.deviceMemory, data, acc.count);
////    for (size_t i = 0; i < acc.count; ++i) {
////        devData[i] = data[i];
////    }
////    vk.device->unmapMemory(bufData.deviceMemory);
//    return bufData;
//}
//
//void tryFillAttributeBuffer(
//        VulkanContext const& vk, entt::resource<tinygltf::Model> const& model, vk::raii::su::BufferData& bufData, std::string const& attrName,
//        uint32_t modelStride, vk::DeviceSize shaderStride, vk::DeviceSize offset
//) {
//    tinygltf::Primitive& primitive = model->meshes.front().primitives.front();
//    // Check if this model has a corresponding attribute by name
//    auto it = primitive.attributes.find(attrName);
//    if (it == primitive.attributes.end()) return;
//
//    uint32_t attrIdx = it->second;
//    tinygltf::Accessor& acc = model->accessors.at(attrIdx);
//    tinygltf::BufferView& view = model->bufferViews.at(acc.bufferView);
//    tinygltf::Buffer& buf = model->buffers.at(view.buffer);
//    auto modelData = reinterpret_cast<std::byte*>(buf.data.data()) + view.byteOffset + acc.byteOffset;
//    auto devData = static_cast<std::byte*>(bufData.deviceMemory->mapMemory(0, view.byteLength)) + offset;
//    for (uint32_t i = 0; i < acc.count; i++) {
//        std::memcpy(devData, modelData, modelStride);
//        modelData += modelStride;
//        devData += shaderStride;
//    }
//    bufData.deviceMemory->unmapMemory();
//}

void render_opaque(App& app) {
//    auto& vk = app.globalCtx.at<VulkanContext>();
//    vk.cmdBufs->front().setViewport(0, vk::Viewport(0.0f, 0.0f,
//                                                    static_cast<float>(vk.surfData->extent.width), static_cast<float>(vk.surfData->extent.height),
//                                                    0.0f, 1.0f));
//    vk.cmdBufs->front().setScissor(0, vk::Rect2D({}, vk.surfData->extent));
//
//    for (auto& [handle, pipeline]: vk.modelPipelines) {
//        Shader const& vertShader = pipeline.shaders[0];
//        vk.cmdBufs->front().bindPipeline(vk::PipelineBindPoint::eGraphics, **pipeline.value);
//        auto renderCtx = app.renderWorld.ctx().at<RenderContext>();
//        auto playerIt = app.renderWorld.view<const Position, const Look, const Player>().each();
//        for (auto [ent, pos, look, player]: playerIt) {
//            if (player.possessionId != renderCtx.possessionId) continue;
//
//            CameraUpload camera{
//                    .view = toShader(calcView(pos, look)),
//                    .proj = toShader(calcProj(vk.surfData->extent)),
//                    .clip = toShader(ClipMat),
//                    .camPos = toShader(pos)
//            };
//            vk::raii::su::copyToDevice(*pipeline.uniforms.find({0, 0})->second.deviceMemory, camera);
//        }
//
//        SceneUpload scene{
//                .lightDir = {1.0f, 1.0f, -1.0f, 0.0f},
//                .exposure = 4.5f,
//                .gamma = 2.2f,
//                .prefilteredCubeMipLevels = 0.0f,
//                .scaleIBLAmbient = 1.0f,
//                .debugViewInputs = 0,
//                .debugViewEquation = 0
//        };
//
//        vk::raii::su::copyToDevice(*pipeline.uniforms.find({0, 1})->second.deviceMemory, scene);
//
//        uint32_t drawIdx;
//        auto modelView = app.renderWorld.view<const Position, const Orientation, const Material, const ModelHandle>();
//        // We store data per model in a dynamic UBO to save memory
//        // This way we only have one upload
//        drawIdx = 0;
//        for (auto [ent, pos, orien, material, modelHandle]: modelView.each()) {
//            vk.modelUpload[drawIdx] = {toShader(calcModel(pos))};
//            vk.materialUpload[drawIdx] = material;
//            drawIdx++;
//        }
//
//        void* mappedModelData = pipeline.uniforms.find({2, 0})->second.deviceMemory->mapMemory(0, vk.modelUpload.mem_size());
//        memcpy(mappedModelData, vk.modelUpload.data(), vk.modelUpload.mem_size());
//        pipeline.uniforms.find({2, 0})->second.deviceMemory->unmapMemory();
//
//        void* mappedMaterialData = pipeline.uniforms.find({2, 1})->second.deviceMemory->mapMemory(0, vk.materialUpload.mem_size());
//        memcpy(mappedMaterialData, vk.materialUpload.data(), vk.materialUpload.mem_size());
//        pipeline.uniforms.find({2, 1})->second.deviceMemory->unmapMemory();
//
//        // TODO: is this same order?
//        drawIdx = 0;
//        for (auto [ent, pos, orien, _, modelHandle]: modelView.each()) {
//            entt::resource<Model> model;
//            ModelBuffers* rawModelBuffers;
//            auto modelBufIt = vk.modelBufData.find(modelHandle.value);
//            // Check if we need to create a vertex buffer for this model
//            if (modelBufIt == vk.modelBufData.end()) {
//                auto [assetIt, wasAssetAdded] = app.modelAssets.load(modelHandle.value, "models/Cube.glb");
//                GAME_ASSERT(wasAssetAdded);
//                model = assetIt->second;
//                GAME_ASSERT(model);
//
//                auto vertCount = static_cast<uint32_t>(model->accessors[model->meshes.front().primitives.front().attributes.at(PositionAttr)].count);
//                vk::raii::su::BufferData vertBufData{*vk.physDev, *vk.device, vertCount * vertShader.vertAttrStride,
//                                                     vk::BufferUsageFlagBits::eVertexBuffer};
//                for (auto& [layout, attr]: vertShader.vertAttrs) {
//                    tryFillAttributeBuffer(vk, model, vertBufData, attr.name, attr.size, vertShader.vertAttrStride, attr.offset);
//                }
//                auto [addedIt, wasBufAdded] = vk.modelBufData.emplace(modelHandle.value, ModelBuffers{
//                        createIndexBufferData<uint16_t>(vk, model),
//                        std::move(vertBufData)
//                });
//                GAME_ASSERT(wasBufAdded);
//                rawModelBuffers = &addedIt->second;
//            } else {
//                model = app.modelAssets[modelHandle.value];
//                rawModelBuffers = &modelBufIt->second;
//            }
//            if (rawModelBuffers) {
//                vk.cmdBufs->front().bindVertexBuffers(0, **rawModelBuffers->vertBufData.buffer, {0});
//                vk.cmdBufs->front().bindIndexBuffer(**rawModelBuffers->indexBufData.buffer, 0, vk::IndexType::eUint16);
//                std::array<uint32_t, 2> dynamicOffsets{
//                        static_cast<uint32_t>(drawIdx * vk.modelUpload.block_size()),
//                        static_cast<uint32_t>(drawIdx * vk.materialUpload.block_size())
//                };
//
//                std::vector<vk::DescriptorSet> proxyDescSets;
//                proxyDescSets.reserve(pipeline.descSets.size());
//                for (auto& descSet: pipeline.descSets) proxyDescSets.push_back(*descSet);
//
//                vk.cmdBufs->front().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, **pipeline.layout, 0u, proxyDescSets, dynamicOffsets);
//                auto indexCount = static_cast<uint32_t>(model->accessors[model->meshes.front().primitives.front().indices].count);
//                vk.cmdBufs->front().drawIndexed(indexCount, 1, 0, 0, 0);
//            }
//            drawIdx++;
//        }
//    }
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
        clock_delta_t avgFrameTime = diagnostics.getAvgFrameTime();
        ImGui::Text("%.3f ms/frame (%.1f FPS)", ms_t(avgFrameTime).count(), 1.0 / sec_t(avgFrameTime).count());
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

void render_imgui(App& app) {
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
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(*vk.commandBuffers[vk.currentSwapchainImageIndex]));
}
