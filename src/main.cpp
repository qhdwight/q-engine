#include "physics.hpp"
#include "state.hpp"
#include "render.hpp"

#include <entt/entt.hpp>

#include <optional>
#include <iostream>

int main() {
    try {
        entt::registry reg;
        auto ent = reg.create();
        reg.emplace_or_replace<position>(ent, 0.0, 0.0, 0.0);

        std::unique_ptr<Render> renderEngine = getRenderEngine();
        while (renderEngine->isActive()) {
            renderEngine->render(reg);
            glfwPollEvents();
        }
    }
    catch (vk::SystemError& err) {
        std::cout << "vk::SystemError: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (std::exception& err) {
        std::cerr << "std::exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
