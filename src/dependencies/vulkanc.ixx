module;

#include <vulkan/vulkan.h>

export module vulkanc;

export {

    using ::VkBool32;
    using ::VkDebugUtilsMessageSeverityFlagBitsEXT;
    using ::VkDebugUtilsMessageTypeFlagsEXT;
    using ::VkDebugUtilsMessengerCallbackDataEXT;
    using ::VkFormat;
    using ::vkGetDeviceProcAddr;
    using ::vkGetInstanceProcAddr;
    using ::VkImage;
    using ::VkImageCreateInfo;
    using ::VkSurfaceKHR;

}// namespace vkc
