#include "inspector.hpp"

#include <vector>

#include <imgui.h>

#include "state.hpp"

using namespace entt::literals;

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
            if (ImGui::TreeNode(&ent, "#%d", ent)) {
//                for (auto [id, storage]: app.logicWorld.storage()) {
//                    if (storage.contains(ent)) {
//                        if (id == "position"_hs) {
//                            std::array<float, 3> position{};
//                            ImGui::InputFloat3("Position", position.data());
//                        }
//                    }
//                }
                for (auto [posEnt, pos]: app.logicWorld.view<Position>().each()) {
                    if (posEnt != ent) continue;

                    ImGui::InputDouble("X", &pos.x);
                    ImGui::InputDouble("Y", &pos.y);
                    ImGui::InputDouble("Z", &pos.z);
                }
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
}
