#pragma once

// Copyright(c) 2019, NVIDIA CORPORATION. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "vulkan/vulkan.hpp"

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <iostream>
#include <limits>
#include <map>

namespace vk::su {
    const uint64_t FenceTimeout = 100000000;

    void setImageLayout(
            vk::CommandBuffer const& commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldImageLayout,
            vk::ImageLayout newImageLayout,
            uint32_t levelCount = 1, uint32_t layerCount = 1, uint32_t baseArrayLevel = 0, uint32_t baseMipLevel = 0);

    struct WindowData {
        WindowData(GLFWwindow* wnd, std::string const& name, vk::Extent2D const& extent);

        WindowData(const WindowData&) = delete;

        WindowData(WindowData&& other) noexcept;

        WindowData& operator=(WindowData&& other) noexcept;

        ~WindowData() noexcept;

        GLFWwindow* handle;
        std::string name;
        vk::Extent2D extent;
    };

    WindowData createWindow(std::string const& windowName, vk::Extent2D const& extent);

    class MonochromeImageGenerator {
    public:
        MonochromeImageGenerator(std::array<unsigned char, 3> const& rgb);

        void operator()(void* data, vk::Extent2D const& extent) const;

    private:
        std::array<unsigned char, 3> const& m_rgb;
    };

    VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                               VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                               VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                                               void* /*pUserData*/ );

    uint32_t findGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperties);

    std::pair<uint32_t, uint32_t> findGraphicsAndPresentQueueFamilyIndex(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR const& surface);

    uint32_t
    findMemoryType(vk::PhysicalDeviceMemoryProperties const& memoryProperties, uint32_t typeBits, vk::MemoryPropertyFlags requirementsMask);

    std::vector<char const*> gatherExtensions(std::vector<std::string> const& extensions
#if !defined( NDEBUG )
            , std::vector<vk::ExtensionProperties> const& extensionProperties
#endif
    );

    std::vector<char const*> gatherLayers(std::vector<std::string> const& layers
#if !defined( NDEBUG )
            , std::vector<vk::LayerProperties> const& layerProperties
#endif
    );

    std::vector<std::string> getDeviceExtensions();

    std::vector<std::string> getInstanceExtensions();

    vk::DebugUtilsMessengerCreateInfoEXT makeDebugUtilsMessengerCreateInfoEXT();

#if defined( NDEBUG )
    vk::StructureChain<vk::InstanceCreateInfo>
#else

    vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
#endif
    makeInstanceCreateInfoChain(vk::ApplicationInfo const& applicationInfo,
                                std::vector<char const*> const& layers,
                                std::vector<char const*> const& extensions);

    vk::PresentModeKHR pickPresentMode(std::vector<vk::PresentModeKHR> const& presentModes);

    vk::SurfaceFormatKHR pickSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& formats);

}  // namespace vk
