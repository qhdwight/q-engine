#include "physics.hpp"
#include "state.hpp"
#include "render.hpp"
#include "vulkan_render.hpp"
#include "octree.hpp"

#include <entt/entt.hpp>

#include <optional>
#include <iostream>
#include <chrono>

int main() {
    try {
        octree<int> oct;
        entt::registry reg;
        for (int i = 0; i < 3; ++i) {
            auto cubeEnt = reg.create();
            reg.emplace<position>(cubeEnt, i * 3.0, 0.0, 0.0);
            reg.emplace<rotation>(cubeEnt, 1.0, 0.0, 0.0, 0.0);
        }

        auto worldEnt = reg.create();
        reg.emplace<VulkanData>(worldEnt);
        reg.emplace<Render>(worldEnt, true);
        world world{std::move(reg), worldEnt};

        auto start = std::chrono::steady_clock::now();
        while (world.reg.get<Render>(worldEnt).keepOpen) {
            auto now = std::chrono::steady_clock::now();
            long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count();
            world.reg.emplace_or_replace<timestamp>(worldEnt, ns);
            tryRenderVulkan(world);
        }
    }
    catch (std::exception const& err) {
        std::cerr << "exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
