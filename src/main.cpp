#include "physics.hpp"
#include "state.hpp"
#include "render.hpp"

#include <entt/entt.hpp>

#include <optional>
#include <iostream>
#include <chrono>

int main() {
    try {
        entt::registry reg;
        for (int i = 0; i < 2; ++i) {
            auto cubeEnt = reg.create();
            reg.emplace_or_replace<position>(cubeEnt, i * 3.0, 0.0, 0.0);
            reg.emplace_or_replace<rotation>(cubeEnt, 1.0, 0.0, 0.0, 0.0);
        }
        auto gameEnt = reg.create();
        world world{std::move(reg), gameEnt};

        auto start = std::chrono::steady_clock::now();
        std::unique_ptr<Render> renderEngine = getRenderEngine();
        while (renderEngine->isActive()) {
            auto now = std::chrono::steady_clock::now();
            long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count();
            world.reg.emplace_or_replace<timestamp>(gameEnt, ns);
            renderEngine->render(world);
            glfwPollEvents();
        }
    }
    catch (std::exception const& err) {
        std::cerr << "exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
