#include "inspector.hpp"

#include <imgui.h>

#include "app.hpp"
#include "state.hpp"

template<typename TComp>
void renderComponent(App& app, entt::entity ent) {
    for (auto [compEnt, comp]: app.logicWorld.view<TComp>().each()) {
        if (compEnt != ent) continue;

        entt::meta_type const& type = entt::resolve<TComp>();

        for (auto field: type.data()) {
            entt::meta_prop tooltip = field.prop("tooltip"_hs);
            if (!tooltip) continue;

            char const* prettyName = tooltip.value().cast<std::string_view>().data();
            entt::meta_type const& fieldType = field.type();
            void* fieldData = field.get(comp).data();
            if (fieldType == entt::resolve<double>()) {
                ImGui::InputDouble(prettyName, static_cast<double*>(fieldData));
            } else if (fieldType == entt::resolve<int>()) {
                ImGui::InputInt(prettyName, static_cast<int*>(fieldData));
            } else if (fieldType == entt::resolve<vec4f>()) {
                ImGui::InputFloat4(prettyName, static_cast<float*>(fieldData));
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
                renderComponent<Material>(app, ent);

                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
}
