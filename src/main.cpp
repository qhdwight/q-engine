#include "state.hpp"
#include "input.hpp"
#include "player/player.hpp"
#include "physics/physics.hpp"
#include "graphics/render.hpp"

int main() {
    try {
        register_reflection();

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
        app.logicWorld.emplace<GroundedPlayerMove>(playerEnt, GroundedPlayerMove{
                .gravity = 30.0,
                .walkSpeed = 5.0,
                .runSpeed = 15.0,
                .fwdSpeed = 20.0,
                .sideSpeed = 20.0,
                .airSpeedCap = 2.0,
                .airAccel = 20.0,
                .maxAirSpeed = 17.0,
                .accel = 15.0,
                .friction = 10.0,
                .frictionCutoff = 0.1,
                .jumpSpeed = 8.5,
                .stopSpeed = 1.0,
        });
//        app.logicWorld.emplace<FlyPlayerMove>(playerEnt, 5.0);
        app.logicWorld.emplace<MoveStats>(playerEnt);

        auto playerDef = edyn::rigidbody_def{
                .kind = edyn::rigidbody_kind::rb_dynamic,
                .position = {0.0, 0.0, 5.0},
                .gravity = edyn::vector3_zero,
                .shape = edyn::capsule_shape{.radius = 0.5, .half_length = 0.5, .axis = edyn::coordinate_axis::z},
                .continuous_contacts = true,
                .presentation = false,
                .sleeping_disabled = true,
        };
        playerDef.material->friction = 0.0;
        edyn::make_rigidbody(playerEnt, app.logicWorld, playerDef);

        auto pickupEnt = app.logicWorld.create();
        app.logicWorld.emplace<ItemPickup>(pickupEnt, "M4"_hs);
        app.logicWorld.emplace<ModelHandle>(pickupEnt, "M4"_hs);

        auto floor_def = edyn::rigidbody_def();
        floor_def.kind = edyn::rigidbody_kind::rb_static;
        floor_def.material->restitution = 1;
        floor_def.material->friction = 0.5;
        floor_def.shape = edyn::plane_shape{.normal = {0, 0, 1}};
        edyn::make_rigidbody(app.logicWorld, floor_def);

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

        app.logicWorld.emplace<Timestamp>(playerEnt, steady_clock_t::now(), clock_delta_t::zero());
        while (app.globalCtx.at<WindowContext>().keepOpen) {

            clock_point_t now = steady_clock_t::now();
            clock_point_t prevPoint = app.logicWorld.get<Timestamp>(playerEnt).point;
            clock_delta_t delta = now - prevPoint;
            app.logicWorld.emplace_or_replace<Timestamp>(playerEnt, now, delta);
            auto& diagnostics = app.globalCtx.at<DiagnosticResource>();
            diagnostics.addFrameTime(delta);

            auto modelView = app.logicWorld.view<Position, Orientation, Material, ModelHandle>();
            int i = -1;
            for (auto [ent, pos, orien, material, modelHandle]: modelView.each()) {
                scalar add = std::cos(sec_t(delta).count());
                scalar x_pos = i++ * 3.0;
                app.logicWorld.emplace_or_replace<Position>(ent, x_pos, 16.0, add - 1);
            }

            inputPlugin->execute(app);

            World& cmdWorld = app.cmdWorldHistory.peek();
            cmdWorld.clear();
            for (auto [ent, player, input]: app.logicWorld.view<Player, Input>().each()) {
                auto actualEnt = cmdWorld.create(ent);
                GAME_ASSERT(actualEnt == ent);
                cmdWorld.emplace<Input>(ent);
            }

            playerControllerPlugin->execute(app);
            physicsPlugin->execute(app);

            app.renderWorld.clear();
            app.renderWorld.ctx().emplace<RenderContext>(app.logicWorld.ctx().at<LocalContext>().possessionId);
            for (auto [ent, pos, look, player]: app.logicWorld.view<const Position, const Look, const Player>().each()) {
                auto actualEnt = app.renderWorld.create(ent);
                GAME_ASSERT(actualEnt == ent);
                app.renderWorld.emplace<Position>(ent, pos);
                app.renderWorld.emplace<Look>(ent, look);
                app.renderWorld.emplace<Player>(ent, player);
            }
            for (auto [ent, pos, orien, material, modelHandle]: modelView.each()) {
                auto actualEnt = app.renderWorld.create(ent);
                GAME_ASSERT(actualEnt == ent);
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
