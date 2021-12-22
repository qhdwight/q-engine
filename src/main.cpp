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
        auto cubeEnt = reg.create();
        auto start = std::chrono::steady_clock::now();
        reg.emplace_or_replace<position>(cubeEnt, 0.0, 0.0, 0.0);
        reg.emplace_or_replace<quaternion>(cubeEnt, 1.0, 0.0, 0.0, 0.0);
        auto gameEnt = reg.create();
        world world{std::move(reg), gameEnt};

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
