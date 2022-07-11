#include "inspector.hpp"

#include <imgui.h>

#include "app.hpp"
#include "state.hpp"

template<typename TComp>
void renderComponent(App& app, entt::entity ent) {
    for (auto [compEnt, comp]: app.logicWorld.view<TComp>().each()) {
        if (compEnt != ent) continue;

        for (auto data: entt::resolve<TComp>().data()) {
            if (data.type() == entt::resolve<double>()) {
                ImGui::InputDouble("F", static_cast<double*>(data.get(comp).data()));
            }
        }
    }
}

void renderImGuiInspector(App& app) {
    auto& vk = app.globalCtx.at<VulkanContext>();

    static std::vector<entt::entity> sortedEntities;
    sortedEntities.clear();
    sortedEntities.reserve(app.logicWorld.size());
    app.logicWorld.each([](entt::entity ent) { sortedEntities.push_back(ent); });
    std::sort(sortedEntities.begin(), sortedEntities.end());
    static bool isOpen = true;
    isOpen = ImGui::Begin("Entity Inspector", &isOpen);
    if (isOpen) {
        for (entt::entity const& ent: sortedEntities) {
            if (ImGui::TreeNode(&ent, "#%u", ent)) {
//                for (auto [possessionId, storage]: app.logicWorld.storage()) {
//                    if (storage.contains(ent)) {
//                        if (possessionId == "position"_hs) {d
//                            std::array<float, 3>fPosition", position.data());
//                        }
//                    }
//                }

                renderComponent<Position>(app, ent);

//                entt::meta<Material>().data<&Material::alphaMask>("alphaMask"_hs);
                for (auto [compEnt, material]: app.logicWorld.view<Material>().each()) {
                    if (compEnt != ent) continue;

//                    ImGui::InputFloat3("baseColorFactor", material.baseColorFactor.data());
//                    ImGui::InputFloat3("emissiveFactor", material.emissiveFactor.data());
//                    ImGui::InputFloat3("diffuseFactor", material.diffuseFactor.data());
//                    ImGui::InputFloat3("specularFactor", material.specularFactor.data());
//                    ImGui::InputFloat("workflow", &material.workflow);
//                    ImGui::InputInt("baseColorTextureSet", &material.baseColorTextureSet);
//                    ImGui::InputInt("physicalDescriptorTextureSet", &material.physicalDescriptorTextureSet);
//                    ImGui::InputInt("normalTextureSet", &material.normalTextureSet);
//                    ImGui::InputInt("occlusionTextureSet", &material.occlusionTextureSet);
//                    ImGui::InputInt("emissiveTextureSet", &material.emissiveTextureSet);
//                    ImGui::InputFloat("metallicFactor", &material.metallicFactor);
//                    ImGui::InputFloat("roughnessFactor", &material.roughnessFactor);
//                    ImGui::InputFloat("alphaMask", &material.alphaMask);
//                    ImGui::InputFloat("alphaMaskCutoff", &material.alphaMaskCutoff);
//                    for (entt::meta_data const& data: entt::resolve<Material>().data()) {
//                        if (data.possessionId() == "float"_hs) {
//                            data.set(material, 0.0f);
//                        }
//                    }
                }

                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
}
