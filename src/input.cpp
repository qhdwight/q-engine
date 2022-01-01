#include "input.hpp"
#include "vulkan_render.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>

void input(World& world) {
    auto vkPtr = world.reg.try_get<VulkanData>(world.sharedEnt);
    if (!vkPtr) return;

    VulkanData& vk = *vkPtr;
    if (!vk.surfData) return;

    GLFWwindow* window = vk.surfData->window.handle;
    if (glfwRawMouseMotionSupported()) glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    for (auto[ent, input]: world.reg.view<Input>().each()) {
        glm::dvec2 prevMouse = input.mouse;
        glfwGetCursorPos(window, &input.mouse.x, &input.mouse.y);
        glm::dvec2 delta = input.mouse - prevMouse;
        auto orienPtr = world.reg.try_get<orientation>(ent);
        if (orienPtr) {
            orientation& orien = *orienPtr;
            delta *= 0.005;
            orien = glm::angleAxis(delta.x, glm::dvec3{0.0, 1.0, 0.0}) * glm::angleAxis(delta.y, glm::dvec3{0.0, 0.0, 1.0}) * orien;
        }
    }
}
