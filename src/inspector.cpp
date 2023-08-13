#include "inspector.hpp"

#include "app.hpp"
#include "state.hpp"

template<std::copyable TComp>
void renderValue(TComp& comp, entt::meta_data const& field, std::string_view name) {
    entt::meta_type const& fieldType = field.type();
    char const* nameCStr = name.data();
    if (fieldType == entt::resolve<double>()) {
        auto d = field.get(comp).template cast<double>();
        ImGui::InputDouble(nameCStr, &d);
        field.set(comp, d);
    } else if (fieldType == entt::resolve<int>()) {
        auto i = field.get(comp).template cast<int>();
        ImGui::InputInt(nameCStr, &i);
        field.set(comp, i);
    } else if (fieldType == entt::resolve<vec3f>()) {
        auto v = field.get(comp).template cast<vec3f>();
        ImGui::InputFloat3(nameCStr, &v.x);
        field.set(comp, v);
    } else if (fieldType == entt::resolve<vec4f>()) {
        auto v = field.get(comp).template cast<vec4f>();
        ImGui::InputFloat4(nameCStr, &v.x);
        field.set(comp, v);
    } else if (fieldType == entt::resolve<vec3>()) {
        auto v = field.get(comp).template cast<vec3>();
        ImGui::InputDouble(nameCStr, &v.x);
        ImGui::InputDouble(nameCStr, &v.y);
        ImGui::InputDouble(nameCStr, &v.z);
        field.set(comp, v);
    }
}

template<std::copyable TComp>
void renderComponent(App& app, entt::entity ent) {
    for (auto [compEnt, comp]: app.logicWorld.view<TComp>().each()) {
        if (compEnt != ent) continue;

        entt::meta_type const& type = entt::resolve<TComp>();

        for (entt::meta_data const& field: type.data()) {
            entt::meta_prop const& tooltip = field.prop("display_name"_hs);
            if (!tooltip) continue;

            auto name = tooltip.value().cast<std::string_view>();
            renderValue(comp, field, name);
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
                renderComponent<GroundedPlayerMove>(app, ent);
                renderComponent<MoveStats>(app, ent);

                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
}
