#include <chrono>
#include <optional>
#include <iostream>

#include <entt/entt.hpp>
#include <glm/gtc/quaternion.hpp>

#include "state.hpp"
#include "input.hpp"
#include "render.hpp"
#include "player.hpp"
#include "vulkan_render.hpp"

int main() {
    try {
        entt::registry reg;
        for (int i = 0; i < 3; ++i) {
            auto cubeEnt = reg.create();
            reg.emplace<Cube>(cubeEnt);
            reg.emplace<position>(cubeEnt);
            reg.emplace<orientation>(cubeEnt, glm::dquat{1.0, 0.0, 0.0, 0.0});
        }
        auto playerEnt = reg.create();
        reg.emplace<Player>(playerEnt);
        reg.emplace<position>(playerEnt, glm::dvec3{0.0, 0.0, 0.0});
        reg.emplace<look>(playerEnt, glm::dvec3{0.0, 0.0, 0.0});
        reg.emplace<Input>(playerEnt);
        reg.emplace<UI>(playerEnt);

        auto worldEnt = reg.create();
        reg.emplace<VulkanResource>(worldEnt);
        reg.emplace<WindowResource>(worldEnt, true, false);
        World world{std::move(reg), worldEnt};

        auto start = std::chrono::steady_clock::now();
        world->emplace<timestamp>(world.sharedEnt, 0, 0);
        while (world->get<WindowResource>(worldEnt).keepOpen) {
            auto now = std::chrono::steady_clock::now();
            long long prevNs = world->get<timestamp>(world.sharedEnt).ns;
            long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count();
            world->emplace_or_replace<timestamp>(worldEnt, ns, ns - prevNs);
            for (auto ent: world->view<position, orientation, Cube>()) {
                double add = std::cos(static_cast<double>(ns) / 1e9);
                double x_pos = ((int) ent - 0) * 3.0;
                world->emplace_or_replace<position>(ent, glm::dvec3{x_pos, 10.0, add - 1});
            }
            input(world);
            modify(world);
            render(world);
        }
    }
    catch (std::exception const& err) {
        std::cerr << "exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
