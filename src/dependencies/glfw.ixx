module;

#include <GLFW/glfw3.h>

export module glfw;

export {
    using ::glfwCreateWindow;
    using ::glfwCreateWindowSurface;
    using ::glfwDestroyWindow;
    using ::glfwGetFramebufferSize;
    using ::glfwGetRequiredInstanceExtensions;
    using ::glfwGetVersionString;
    using ::glfwInit;
    using ::glfwPollEvents;
    using ::glfwSetErrorCallback;
    using ::glfwVulkanSupported;
    using ::GLFWwindow;
    using ::glfwWindowHint;
    using ::glfwWindowShouldClose;
}

export namespace glfw {

    constexpr int CLIENT_API = GLFW_CLIENT_API;
    constexpr int NO_API = GLFW_NO_API;

}// namespace glfw

