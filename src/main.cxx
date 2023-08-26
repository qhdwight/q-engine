import app;
import render;

import std;
import entt;
import gltf;

using namespace entt::literals;

constexpr int EXIT_SUCCESS = 0;
constexpr int EXIT_FAILURE = 0;

int main() {
    try {
        entt::resource_cache <gltf::Model, gltf::ModelLoader> models;
        std::ifstream stream("assets/models/Cube.glb", std::ios::binary);
        models.load("cube"_hs, stream);

        App app;
        auto renderPlugin = app.makePlugin<VulkanRenderPlugin>();
        while (app.globalContext.get<WindowContext>().keepOpen) {
            renderPlugin->execute(app);
        }
    } catch (std::exception const &err) {
        std::cerr << "exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
