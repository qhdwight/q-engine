#include <chrono>
#include <optional>
#include <iostream>

#include <entt/entt.hpp>

#include "state.hpp"
#include "input.hpp"
#include "player.hpp"
#include "physics.hpp"
#include "vulkan_render.hpp"

int main() {
    try {
        World world;
        Resources resources;
        for (int i = 0; i < 3; ++i) {
            auto cubeEnt = world.create();
            world.emplace<Cube>(cubeEnt);
            world.emplace<Position>(cubeEnt);
            world.emplace<Orientation>(cubeEnt, 1.0, 0.0, 0.0, 0.0);
        }
        auto playerEnt = world.create();
        world.emplace<Player>(playerEnt);
        world.emplace<Position>(playerEnt, 0.0, 0.0, 0.0);
        world.emplace<Look>(playerEnt);
        world.emplace<Input>(playerEnt);
        world.emplace<UI>(playerEnt);

//        reg.emplace<PhysicsResource>(worldEnt);
        resources.emplace<DiagnosticResource>();

        ExecuteContext ctx{world, resources};

        PhysicsPlugin physics;
        VulkanRenderPlugin render;
        physics.build(ctx);
        render.build(ctx);

        auto start = std::chrono::steady_clock::now();
        world.emplace<Timestamp>(playerEnt, 0, 0);
        while (resources.at<WindowResource>().keepOpen) {
            auto now = std::chrono::steady_clock::now();
            ns_t prevNs = world.get<Timestamp>(playerEnt).ns;
            ns_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count();
            ns_t deltaNs = ns - prevNs;
            world.emplace_or_replace<Timestamp>(playerEnt, ns, deltaNs);
            auto& diagnostics = resources.at<DiagnosticResource>();
            diagnostics.addFrameTime(deltaNs);
            for (auto ent: world.view<Position, Orientation, Cube>()) {
                scalar add = std::cos(static_cast<double>(ns) / 1e9);
                scalar x_pos = ((int) ent - 0) * 3.0;
                world.emplace_or_replace<Position>(ent, x_pos, 10.0, add - 1);
            }
            input(ctx);
            modify(ctx);
            render.execute(ctx);
            physics.execute(ctx);
        }
    }
    catch (std::exception const& err) {
        std::cerr << "exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
