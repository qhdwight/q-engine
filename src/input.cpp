#include "input.hpp"

#include <GLFW/glfw3.h>

#include "state.hpp"
#include "vulkan_render.hpp"

void input(ExecuteContext& ctx) {
    // TODO:arch restructure so it is graphics API agnostic
    auto vkPtr = ctx.resources.find<VulkanResource>();
    if (!vkPtr) return;

    VulkanResource& vk = *vkPtr;
    if (!vk.surfData) return;

    auto& window = ctx.resources.at<WindowResource>();

    GLFWwindow* glfwWindow = vk.surfData->window.handle;
    if (glfwRawMouseMotionSupported()) glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    auto it = ctx.world.view<UI>().each();
    bool uiVisible = std::any_of(it.begin(), it.end(), [](std::tuple<entt::entity, UI> const& t) { return std::get<1>(t).visible; });
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, uiVisible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    bool isFocused = glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED);
    for (auto [ent, input, ui]: ctx.world.view<Input, UI>().each()) {
        vec2 prevMouse = input.cursor;
        glfwGetCursorPos(glfwWindow, &input.cursor.x, &input.cursor.y);
        if (window.isFocused == isFocused && !uiVisible) {
            input.cursorDelta = (input.cursor - prevMouse) * 0.005;
        } else { // prevent snapping when tabbing back in
            input.cursorDelta = {};
        }
        window.isFocused = isFocused;

        input.move = {
                (glfwGetKey(glfwWindow, GLFW_KEY_D) ? 1.0 : 0.0) + (glfwGetKey(glfwWindow, GLFW_KEY_A) ? -1.0 : 0.0),
                (glfwGetKey(glfwWindow, GLFW_KEY_W) ? 1.0 : 0.0) + (glfwGetKey(glfwWindow, GLFW_KEY_S) ? -1.0 : 0.0),
                (glfwGetKey(glfwWindow, GLFW_KEY_SPACE) ? 1.0 : 0.0) + (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT) ? -1.0 : 0.0)
        };
        input.lean = (glfwGetKey(glfwWindow, GLFW_KEY_E) ? 1.0 : 0.0) + (glfwGetKey(glfwWindow, GLFW_KEY_Q) ? -1.0 : 0.0);
        input.menu.previous = input.menu.current;
        input.menu.current = glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE);
        if (input.menu.current != input.menu.previous && input.menu.current) {
            ui.visible = !ui.visible;
        }
    }
}
