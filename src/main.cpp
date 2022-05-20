#include <chrono>
#include <optional>
#include <iostream>

#include <entt/entt.hpp>

#include "state.hpp"
#include "input.hpp"
#include "render.hpp"
#include "player.hpp"
#include "physics.hpp"

int main() {
    try {
        entt::registry reg;
        for (int i = 0; i < 3; ++i) {
            auto cubeEnt = reg.create();
            reg.emplace<Cube>(cubeEnt);
            reg.emplace<Position>(cubeEnt);
            reg.emplace<Orientation>(cubeEnt, 1.0, 0.0, 0.0, 0.0);
        }
        auto playerEnt = reg.create();
        reg.emplace<Player>(playerEnt);
        reg.emplace<Position>(playerEnt, 0.0, 0.0, 0.0);
        reg.emplace<Look>(playerEnt);
        reg.emplace<Input>(playerEnt);
        reg.emplace<UI>(playerEnt);

        auto worldEnt = reg.create();
        reg.emplace<GraphicsResource>(worldEnt);
        reg.emplace<WindowResource>(worldEnt, false, true, false);
//        reg.emplace<PhysicsResource>(worldEnt);
        reg.emplace<DiagnosticResource>(worldEnt);
        World world{std::move(reg), worldEnt};

        auto start = std::chrono::steady_clock::now();
        world->emplace<Timestamp>(world.sharedEnt, 0, 0);
        while (world->get<WindowResource>(worldEnt).keepOpen) {
            auto now = std::chrono::steady_clock::now();
            ns_t prevNs = world->get<Timestamp>(world.sharedEnt).ns;
            ns_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count();
            ns_t deltaNs = ns - prevNs;
            world->emplace_or_replace<Timestamp>(worldEnt, ns, deltaNs);
            auto& diagnostics = world->get<DiagnosticResource>(worldEnt);
            diagnostics.addFrameTime(deltaNs);
            for (auto ent: world->view<Position, Orientation, Cube>()) {
                scalar add = std::cos(static_cast<double>(ns) / 1e9);
                scalar x_pos = ((int) ent - 0) * 3.0;
                world->emplace_or_replace<Position>(ent, x_pos, 10.0, add - 1);
            }
            input(world);
            modify(world);
            render(world);
            physics(world);
        }
    }
    catch (std::exception const& err) {
        std::cerr << "exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
