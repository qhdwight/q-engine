import app;
import render;

import std;

constexpr int EXIT_SUCCESS = 0;
constexpr int EXIT_FAILURE = 0;

int main() {
    try {
        App app;
        auto renderPlugin = app.makePlugin<VulkanRenderPlugin>();
        while (app.globalContext.get<WindowContext>().keepOpen) {
            renderPlugin->execute(app);
        }
    } catch (std::exception const& err) {
        std::cerr << "exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
