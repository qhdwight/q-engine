#include "inspector.hpp"

#include <vector>

#include <imgui.h>

using namespace entt::literals;

void renderImGuiInspector(App& app) {
    auto& vk = app.globalCtx.at<VulkanContext>();

    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    static std::vector<entt::entity> sortedEntities;
    sortedEntities.clear();
    app.renderWorld.each([&](entt::entity ent) { sortedEntities.push_back(ent); });
    std::sort(sortedEntities.begin(), sortedEntities.end());
    static bool open = true;
    if (ImGui::Begin("Inspector", &open, windowFlags)) {
        for (entt::entity const& ent: sortedEntities) {
            if (ImGui::TreeNode(&ent, "#%d", ent)) {
                for (auto [id, storage]: app.logicWorld.storage()) {
                    if (storage.contains(ent)) {
                        if (id == "position"_hs) {
                            std::array<float, 3> position{};
                            ImGui::InputFloat3("Position", position.data());
                        }
                    }
                }
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
}
