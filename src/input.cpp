#include "input.hpp"
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
        if (window.isFocused == isFocused) { // prevent snapping when tabbing back in
            glm::dvec2 delta = input.cursor - prevMouse;
            auto lookPtr = world.reg.try_get<look>(ent);
            if (lookPtr) {
                look& look = *lookPtr;
                delta *= 0.005;
                look.vec += glm::dvec3{-delta.y, delta.x, 0.0};
                look.vec.x = std::clamp(look.vec.x, glm::radians(-90.0), glm::radians(90.0));
            }
        } else {
            window.isFocused = isFocused;
        }
    }
}
