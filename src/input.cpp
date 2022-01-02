#include "input.hpp"
#include "state.hpp"
#include "render.hpp"
#include "vulkan_render.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>

void input(World& world) {
    auto vkPtr = world.reg.try_get<VulkanData>(world.sharedEnt);
    if (!vkPtr) return;

    VulkanData& vk = *vkPtr;
    if (!vk.surfData) return;

    auto& window = world.reg.get<Window>(world.sharedEnt);

    GLFWwindow* glfwWindow = vk.surfData->window.handle;
    if (glfwRawMouseMotionSupported()) glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    bool isFocused = glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED);
    for (auto[ent, input]: world.reg.view<Input>().each()) {
        glm::dvec2 prevMouse = input.cursor;
        glfwGetCursorPos(glfwWindow, &input.cursor.x, &input.cursor.y);
        if (window.isFocused == isFocused) {
            input.cursorDelta = (input.cursor - prevMouse) * 0.005;
        } else { // prevent snapping when tabbing back in
            input.cursorDelta = {};
        }
        window.isFocused = isFocused;

        input.move = {
                (glfwGetKey(glfwWindow, GLFW_KEY_D) ? 1.0 : 0.0) + (glfwGetKey(glfwWindow, GLFW_KEY_A) ? -1.0 : 0.0),
                (glfwGetKey(glfwWindow, GLFW_KEY_SPACE) ? 1.0 : 0.0) + (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT) ? -1.0 : 0.0),
                (glfwGetKey(glfwWindow, GLFW_KEY_W) ? 1.0 : 0.0) + (glfwGetKey(glfwWindow, GLFW_KEY_S) ? -1.0 : 0.0),
        };
    }
}
