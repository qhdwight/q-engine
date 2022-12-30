#include "render.hpp"

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                           VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                           VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                                           void* /*pUserData*/ );