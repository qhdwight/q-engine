#include "input.hpp"

#include <GLFW/glfw3.h>

#include "state.hpp"
#include "graphics/render.hpp"

double getAxis(GLFWwindow* glfwWindow, int positiveKey, int negativeKey) {
    return (glfwGetKey(glfwWindow, positiveKey) ? +1.0 : 0.0) +
           (glfwGetKey(glfwWindow, negativeKey) ? -1.0 : 0.0);
}


void InputPlugin::execute(App& app) {
//    // TODO:arch restructure so it is graphics API agnostic
//    auto vkPtr = app.globalCtx.find<VulkanContext>();
//    if (!vkPtr) return;
//
//    VulkanContext& vk = *vkPtr;
//    if (!vk.surfData) return;
//
//    auto& window = app.globalCtx.at<WindowContext>();
//
//    GLFWwindow* glfwWindow = vk.surfData->window.handle;
//    if (glfwRawMouseMotionSupported()) glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
//
//    auto it = app.logicWorld.view<UI>().each();
//    bool isUiVisible = std::any_of(it.begin(), it.end(), [](std::tuple<entt::entity, UI> const& t) { return std::get<1>(t).isVisible; });
//    glfwSetInputMode(glfwWindow, GLFW_CURSOR, isUiVisible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
//    bool isFocused = glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED);
//
//    std::optional<possesion_id_t> possessionId = app.logicWorld.ctx().at<LocalContext>().possessionId;
//
//    for (auto [ent, player, input, ui]: app.logicWorld.view<Player, Input, UI>().each()) {
//        if (player.possessionId != possessionId) continue;
//
//        vec2 prevMouse = input.cursor;
//        glfwGetCursorPos(glfwWindow, &input.cursor.x, &input.cursor.y);
//        if (window.isFocused == isFocused && !isUiVisible) {
//            input.cursorDelta = (input.cursor - prevMouse) * 0.005;
//        } else { // prevent snapping when tabbing back in
//            input.cursorDelta = {};
//        }
//        window.isFocused = isFocused;
//
//        input.move = {
//                getAxis(glfwWindow, GLFW_KEY_D, GLFW_KEY_A),
//                getAxis(glfwWindow, GLFW_KEY_W, GLFW_KEY_S),
//                getAxis(glfwWindow, GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT)
//        };
////        input.move.y = 1.0;
//        input.lean = getAxis(glfwWindow, GLFW_KEY_E, GLFW_KEY_Q);
//        input.menu.previous = input.menu.current;
//        input.menu.current = glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE);
//        input.jump.previous = input.menu.current;
//        input.jump.current = glfwGetKey(glfwWindow, GLFW_KEY_SPACE);
//        if (input.menu.current != input.menu.previous && input.menu.current) {
//            ui.isVisible = !ui.isVisible;
//        }
//    }
}
