#include "state.hpp"
#include "input.hpp"
#include "player/player.hpp"
#include "physics/physics.hpp"
#include "graphics/render.hpp"

int main() {
    try {
        App app;
        auto inputPlugin = app.makePlugin<InputPlugin>();
        auto renderPlugin = app.makePlugin<VulkanRenderPlugin>();
        auto physicsPlugin = app.makePlugin<PhysicsPlugin>();
        auto playerControllerPlugin = app.makePlugin<PlayerControllerPlugin>();

        app.logicWorld.ctx().emplace<LocalContext>(possesion_id_t{0}, Authority::Client);

        auto playerEnt = app.logicWorld.create();
        app.logicWorld.emplace<Player>(playerEnt, possesion_id_t{0});
        app.logicWorld.emplace<Look>(playerEnt);
        app.logicWorld.emplace<Input>(playerEnt);
        app.logicWorld.emplace<UI>(playerEnt);
        app.logicWorld.emplace<GroundedPlayerMove>(playerEnt);

        auto pickupEnt = app.logicWorld.create();
        app.logicWorld.emplace<ItemPickup>(pickupEnt, "M4"_hs);
        app.logicWorld.emplace<ModelHandle>(pickupEnt, "M4"_hs);

        auto playerDef = edyn::rigidbody_def{
                .kind = edyn::rigidbody_kind::rb_dynamic,
                .gravity = edyn::vector3_zero,
                .shape = edyn::capsule_shape{0.5, 1.0},
                .continuous_contacts = true,
                .presentation = false,
                .sleeping_disabled = true,
        };
        edyn::make_rigidbody(playerEnt, app.logicWorld, playerDef);

        for (int i = 0; i < 3; ++i) {
            auto cubeEnt = app.logicWorld.create();
            app.logicWorld.emplace<ModelHandle>(cubeEnt, "Cube"_hs);
            app.logicWorld.emplace<Position>(cubeEnt);
            app.logicWorld.emplace<Orientation>(cubeEnt, 1.0, 0.0, 0.0, 0.0);
            Material material{
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
            app.logicWorld.emplace<Material>(cubeEnt, material);
        }

        app.globalCtx.emplace<DiagnosticResource>();

        auto start = std::chrono::steady_clock::now();
        app.logicWorld.emplace<Timestamp>(playerEnt, 0, 0);
        while (app.globalCtx.at<WindowContext>().keepOpen) {

            /* ======== Tick ======== */
            auto now = std::chrono::steady_clock::now();
            ns_t prevNs = app.logicWorld.get<Timestamp>(playerEnt).ns;
            ns_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count();
            ns_t deltaNs = ns - prevNs;
            app.logicWorld.emplace_or_replace<Timestamp>(playerEnt, ns, deltaNs);
            auto& diagnostics = app.globalCtx.at<DiagnosticResource>();
            diagnostics.addFrameTime(deltaNs);

            auto modelView = app.logicWorld.view<Position, Orientation, Material, ModelHandle>();
            for (auto [ent, pos, orien, material, modelHandle]: modelView.each()) {
                scalar add = std::cos(static_cast<double>(ns) / 1e9);
                scalar x_pos = ((int) ent - 0) * 3.0;
                app.logicWorld.emplace_or_replace<Position>(ent, x_pos, 10.0, add - 1);
            }
            inputPlugin->execute(app);
            playerControllerPlugin->execute(app);
            physicsPlugin->execute(app);

            app.renderWorld.clear();
            app.renderWorld.ctx().emplace<RenderContext>(app.logicWorld.ctx().at<LocalContext>().possessionId);
            for (auto [ent, pos, look, player]: app.logicWorld.view<const Position, const Look, const Player>().each()) {
                auto actualEnt = app.renderWorld.create(ent);
                assert(actualEnt == ent);
                app.renderWorld.emplace<Position>(ent, pos);
                app.renderWorld.emplace<Look>(ent, look);
                app.renderWorld.emplace<Player>(ent, player);
            }
            for (auto [ent, pos, orien, material, modelHandle]: modelView.each()) {
                auto actualEnt = app.renderWorld.create(ent);
                assert(actualEnt == ent);
                app.renderWorld.emplace<Position>(ent, pos);
                app.renderWorld.emplace<Orientation>(ent, orien);
                app.renderWorld.emplace<ModelHandle>(ent, modelHandle);
                app.renderWorld.emplace<Material>(ent, material);
                app.renderWorld.emplace<ShaderHandle>(ent, "Flat"_hs);
            }
            renderPlugin->execute(app);

            app.cmdWorldHistory.advance();
        }
    }
    catch (std::exception const& err) {
        std::cerr << "exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
