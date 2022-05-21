#include <chrono>
#include <optional>
#include <iostream>

#include <entt/entt.hpp>

#include "state.hpp"
#include "input.hpp"
#include "player.hpp"
#include "physics.hpp"
#include "render.hpp"

using namespace entt::literals;

int main() {
    try {
        App app;
        PhysicsPlugin physics;
        VulkanRenderPlugin render;
        SystemContext logicCtx{app.logicWorld, app.globalCtx};
        physics.build(logicCtx);
        render.build(logicCtx);

        app.sceneAssets.load("M4"_hs, "assets/models/M4.glb");

        app.logicWorld.ctx().emplace<LocalContext>(player_id_t{0});

        auto playerEnt = app.logicWorld.create();
        app.logicWorld.emplace<Player>(playerEnt, player_id_t{0});
        app.logicWorld.emplace<Look>(playerEnt);
        app.logicWorld.emplace<Input>(playerEnt);
        app.logicWorld.emplace<UI>(playerEnt);

        auto player_def = edyn::rigidbody_def();
        player_def.kind = edyn::rigidbody_kind::rb_dynamic;
        player_def.sleeping_disabled = true;
        player_def.shape = edyn::capsule_shape{0.5, 1.0};
        player_def.gravity = edyn::vector3_zero;
        player_def.presentation = false;
        player_def.continuous_contacts = true;
        edyn::make_rigidbody(playerEnt, app.logicWorld, player_def);

        for (int i = 0; i < 3; ++i) {
            auto cubeEnt = app.logicWorld.create();
            app.logicWorld.emplace<Cube>(cubeEnt);
            app.logicWorld.emplace<Position>(cubeEnt);
            app.logicWorld.emplace<Orientation>(cubeEnt, 1.0, 0.0, 0.0, 0.0);
        }

//        reg.emplace<PhysicsResource>(worldEnt);
        app.globalCtx.emplace<DiagnosticResource>();

        auto start = std::chrono::steady_clock::now();
        app.logicWorld.emplace<Timestamp>(playerEnt, 0, 0);
        while (app.globalCtx.at<WindowContext>().keepOpen) {
            auto now = std::chrono::steady_clock::now();
            ns_t prevNs = app.logicWorld.get<Timestamp>(playerEnt).ns;
            ns_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count();
            ns_t deltaNs = ns - prevNs;
            app.logicWorld.emplace_or_replace<Timestamp>(playerEnt, ns, deltaNs);
            auto& diagnostics = app.globalCtx.at<DiagnosticResource>();
            diagnostics.addFrameTime(deltaNs);
            for (auto ent: app.logicWorld.view<Position, Orientation, Cube>()) {
                scalar add = std::cos(static_cast<double>(ns) / 1e9);
                scalar x_pos = ((int) ent - 0) * 3.0;
                app.logicWorld.emplace_or_replace<Position>(ent, x_pos, 10.0, add - 1);
            }
            input(logicCtx);
            modify(logicCtx);
            physics.execute(logicCtx);

            app.renderWorld.clear();
            app.renderWorld.ctx().emplace<RenderContext>(app.logicWorld.ctx().at<LocalContext>().localId);
            for (auto [ent, pos, look, player]: app.logicWorld.view<const Position, const Look, const Player>().each()) {
                auto actualEnt = app.renderWorld.create(ent);
                assert(actualEnt == ent);
                app.renderWorld.emplace<Position>(ent, pos);
                app.renderWorld.emplace<Look>(ent, look);
                app.renderWorld.emplace<Player>(ent, player);
            }
            for (auto [ent, pos, orien]: app.logicWorld.view<const Position, const Orientation, const Cube>().each()) {
                auto actualEnt = app.renderWorld.create(ent);
                assert(actualEnt == ent);
                app.renderWorld.emplace<Position>(ent, pos);
                app.renderWorld.emplace<Orientation>(ent, orien);
                app.renderWorld.emplace<Cube>(ent);
            }
            render.execute({app.renderWorld, app.globalCtx});
        }
        physics.cleanup(logicCtx);
    }
    catch (std::exception const& err) {
        std::cerr << "exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
