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

#include "utils.hpp"

#include "vulkan/vulkan.hpp"

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

namespace vk::su {
    vk::DeviceMemory allocateDeviceMemory(vk::Device const& device,
                                          vk::PhysicalDeviceMemoryProperties const& memoryProperties,
                                          vk::MemoryRequirements const& memoryRequirements,
                                          vk::MemoryPropertyFlags memoryPropertyFlags) {
        uint32_t memoryTypeIndex = findMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryPropertyFlags);

        return device.allocateMemory(vk::MemoryAllocateInfo(memoryRequirements.size, memoryTypeIndex));
    }

    bool contains(std::vector<vk::ExtensionProperties> const& extensionProperties, std::string const& extensionName) {
        auto propertyIterator = std::find_if(extensionProperties.begin(),
                                             extensionProperties.end(),
                                             [&extensionName](vk::ExtensionProperties const& ep) { return extensionName == ep.extensionName; });
        return (propertyIterator != extensionProperties.end());
    }

    vk::CommandPool createCommandPool(vk::Device const& device, uint32_t queueFamilyIndex) {
        vk::CommandPoolCreateInfo commandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndex);
        return device.createCommandPool(commandPoolCreateInfo);
    }

    vk::DebugUtilsMessengerEXT createDebugUtilsMessengerEXT(vk::Instance const& instance) {
        return instance.createDebugUtilsMessengerEXT(vk::su::makeDebugUtilsMessengerCreateInfoEXT());
    }

    std::vector<vk::Framebuffer> createFramebuffers(vk::Device const& device,
                                                    vk::RenderPass& renderPass,
                                                    std::vector<vk::ImageView> const& imageViews,
                                                    vk::ImageView const& depthImageView,
                                                    vk::Extent2D const& extent) {
        vk::ImageView attachments[2];
        attachments[1] = depthImageView;

        vk::FramebufferCreateInfo framebufferCreateInfo(
                vk::FramebufferCreateFlags(), renderPass, depthImageView ? 2 : 1, attachments, extent.width, extent.height, 1);
        std::vector<vk::Framebuffer> framebuffers;
        framebuffers.reserve(imageViews.size());
        for (auto const& view: imageViews) {
            attachments[0] = view;
            framebuffers.push_back(device.createFramebuffer(framebufferCreateInfo));
        }

        return framebuffers;
    }

    std::vector<char const*> gatherExtensions(std::vector<std::string> const& extensions
#if !defined( NDEBUG )
            ,
                                              std::vector<vk::ExtensionProperties> const& extensionProperties
#endif
    ) {
        std::vector<char const*> enabledExtensions;
        enabledExtensions.reserve(extensions.size());
        for (auto const& ext: extensions) {
            assert(std::find_if(extensionProperties.begin(),
                                extensionProperties.end(),
                                [ext](vk::ExtensionProperties const& ep) { return ext == ep.extensionName; }) != extensionProperties.end());
            enabledExtensions.push_back(ext.data());
        }
#if !defined( NDEBUG )
        if (std::find(extensions.begin(), extensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == extensions.end() &&
            std::find_if(extensionProperties.begin(),
                         extensionProperties.end(),
                         [](vk::ExtensionProperties const& ep) { return (strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, ep.extensionName) == 0); }) !=
            extensionProperties.end()) {
            enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
#endif
        return enabledExtensions;
    }

    std::vector<char const*> gatherLayers(std::vector<std::string> const& layers
#if !defined( NDEBUG )
            ,
                                          std::vector<vk::LayerProperties> const& layerProperties
#endif
    ) {
        std::vector<char const*> enabledLayers;
        enabledLayers.reserve(layers.size());
        for (auto const& layer: layers) {
            assert(std::find_if(layerProperties.begin(), layerProperties.end(),
                                [layer](vk::LayerProperties const& lp) { return layer == lp.layerName; }) !=
                   layerProperties.end());
            enabledLayers.push_back(layer.data());
        }
#if !defined( NDEBUG )
        // Enable standard validation layer to find as much errors as possible!
        if (std::find(layers.begin(), layers.end(), "VK_LAYER_KHRONOS_validation") == layers.end() &&
            std::find_if(layerProperties.begin(),
                         layerProperties.end(),
                         [](vk::LayerProperties const& lp) { return (strcmp("VK_LAYER_KHRONOS_validation", lp.layerName) == 0); }) !=
            layerProperties.end()) {
            enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
        }
#endif
        return enabledLayers;
    }

    vk::RenderPass createRenderPass(
            vk::Device const& device, vk::Format colorFormat, vk::Format depthFormat, vk::AttachmentLoadOp loadOp,
            vk::ImageLayout colorFinalLayout) {
        std::vector<vk::AttachmentDescription> attachmentDescriptions;
        assert(colorFormat != vk::Format::eUndefined);
        attachmentDescriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                            colorFormat,
                                            vk::SampleCountFlagBits::e1,
                                            loadOp,
                                            vk::AttachmentStoreOp::eStore,
                                            vk::AttachmentLoadOp::eDontCare,
                                            vk::AttachmentStoreOp::eDontCare,
                                            vk::ImageLayout::eUndefined,
                                            colorFinalLayout);
        if (depthFormat != vk::Format::eUndefined) {
            attachmentDescriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                                depthFormat,
                                                vk::SampleCountFlagBits::e1,
                                                loadOp,
                                                vk::AttachmentStoreOp::eDontCare,
                                                vk::AttachmentLoadOp::eDontCare,
                                                vk::AttachmentStoreOp::eDontCare,
                                                vk::ImageLayout::eUndefined,
                                                vk::ImageLayout::eDepthStencilAttachmentOptimal);
        }
        vk::AttachmentReference colorAttachment(0, vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference depthAttachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
        vk::SubpassDescription subpassDescription(vk::SubpassDescriptionFlags(),
                                                  vk::PipelineBindPoint::eGraphics,
                                                  {},
                                                  colorAttachment,
                                                  {},
                                                  (depthFormat != vk::Format::eUndefined) ? &depthAttachment : nullptr);
        return device.createRenderPass(vk::RenderPassCreateInfo(vk::RenderPassCreateFlags(), attachmentDescriptions, subpassDescription));
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                               VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                               VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                                               void* /*pUserData*/ ) {
#if !defined( NDEBUG )
        if (pCallbackData->messageIdNumber == 648835635) {
            // UNASSIGNED-khronos-Validation-debug-build-warning-message
            return VK_FALSE;
        }
        if (pCallbackData->messageIdNumber == 767975156) {
            // UNASSIGNED-BestPractices-vkCreateInstance-specialuse-extension
            return VK_FALSE;
        }
#endif

        std::cerr << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>( messageSeverity )) << ": "
                  << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>( messageTypes )) << ":\n";
        std::cerr << "\t"
                  << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
        std::cerr << "\t"
                  << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
        std::cerr << "\t"
                  << "message         = <" << pCallbackData->pMessage << ">\n";
        if (0 < pCallbackData->queueLabelCount) {
            std::cerr << "\t"
                      << "Queue Labels:\n";
            for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++) {
                std::cerr << "\t\t"
                          << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
            }
        }
        if (0 < pCallbackData->cmdBufLabelCount) {
            std::cerr << "\t"
                      << "CommandBuffer Labels:\n";
            for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
                std::cerr << "\t\t"
                          << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
            }
        }
        if (0 < pCallbackData->objectCount) {
            std::cerr << "\t"
                      << "Objects:\n";
            for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
                std::cerr << "\t\t"
                          << "Object " << i << "\n";
                std::cerr << "\t\t\t"
                          << "objectType   = " << vk::to_string(static_cast<vk::ObjectType>( pCallbackData->pObjects[i].objectType )) << "\n";
                std::cerr << "\t\t\t"
                          << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
                if (pCallbackData->pObjects[i].pObjectName) {
                    std::cerr << "\t\t\t"
                              << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
                }
            }
        }
        return VK_TRUE;
    }

    uint32_t findGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperties) {
        // get the first index into queueFamilyProperties which supports graphics
        auto graphicsQueueFamilyProperty = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                                                        [](vk::QueueFamilyProperties const& qfp) {
                                                            return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
                                                        });
        assert(graphicsQueueFamilyProperty != queueFamilyProperties.end());
        return static_cast<uint32_t>( std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));
    }

    std::pair<uint32_t, uint32_t> findGraphicsAndPresentQueueFamilyIndex(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR const& surface) {
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        assert(queueFamilyProperties.size() < std::numeric_limits<uint32_t>::max());

        uint32_t graphicsQueueFamilyIndex = findGraphicsQueueFamilyIndex(queueFamilyProperties);
        if (physicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, surface)) {
            return std::make_pair(graphicsQueueFamilyIndex,
                                  graphicsQueueFamilyIndex);  // the first graphicsQueueFamilyIndex does also support presents
        }

        // the graphicsQueueFamilyIndex doesn't support present -> look for an other family index that supports both
        // graphics and present
        for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
            if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>( i ), surface)) {
                return std::make_pair(static_cast<uint32_t>( i ), static_cast<uint32_t>( i ));
            }
        }

        // there's nothing like a single family index that supports both graphics and present -> look for an other family
        // index that supports present
        for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
            if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>( i ), surface)) {
                return std::make_pair(graphicsQueueFamilyIndex, static_cast<uint32_t>( i ));
            }
        }

        throw std::runtime_error("Could not find queues for both graphics or present -> terminating");
    }

    uint32_t
    findMemoryType(vk::PhysicalDeviceMemoryProperties const& memoryProperties, uint32_t typeBits, vk::MemoryPropertyFlags requirementsMask) {
        auto typeIndex = uint32_t(~0);
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if ((typeBits & 1) && ((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)) {
                typeIndex = i;
                break;
            }
            typeBits >>= 1;
        }
        assert(typeIndex != uint32_t(~0));
        return typeIndex;
    }

    std::vector<std::string> getDeviceExtensions() {
        std::vector<std::string> extensions;
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#if defined( VK_USE_PLATFORM_MACOS_MVK )
        extensions.push_back( "VK_KHR_portability_subset" );
#endif
        return extensions;
    }

    std::vector<std::string> getInstanceExtensions() {
        std::vector<std::string> extensions;
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined( VK_USE_PLATFORM_ANDROID_KHR )
        extensions.push_back( VK_KHR_ANDROID_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_IOS_MVK )
        extensions.push_back( VK_MVK_IOS_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_MACOS_MVK )
        extensions.push_back( "VK_EXT_metal_surface" );
        extensions.push_back( "VK_KHR_get_physical_device_properties2" );
#elif defined( VK_USE_PLATFORM_MIR_KHR )
        extensions.push_back( VK_KHR_MIR_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_VI_NN )
        extensions.push_back( VK_NN_VI_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_WAYLAND_KHR )
        extensions.push_back( VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_WIN32_KHR )
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_XCB_KHR )
        extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined( VK_USE_PLATFORM_XLIB_KHR )
        extensions.push_back( VK_KHR_XLIB_SURFACE_EXTENSION_NAME );
#elif defined( VK_USE_PLATFORM_XLIB_XRANDR_EXT )
        extensions.push_back( VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME );
#endif
        return extensions;
    }

    vk::PresentModeKHR pickPresentMode(std::vector<vk::PresentModeKHR> const& presentModes) {
        vk::PresentModeKHR pickedMode = vk::PresentModeKHR::eFifo;
        for (const auto& presentMode: presentModes) {
            if (presentMode == vk::PresentModeKHR::eMailbox) {
                pickedMode = presentMode;
                break;
            }

            if (presentMode == vk::PresentModeKHR::eImmediate) {
                pickedMode = presentMode;
            }
        }
        return pickedMode;
    }

    vk::SurfaceFormatKHR pickSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& formats) {
        assert(!formats.empty());
        vk::SurfaceFormatKHR pickedFormat = formats[0];
        if (formats.size() == 1) {
            if (formats[0].format == vk::Format::eUndefined) {
                pickedFormat.format = vk::Format::eB8G8R8A8Unorm;
                pickedFormat.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
            }
        } else {
            // request several formats, the first found will be used
            vk::Format requestedFormats[] = {vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8Unorm,
                                             vk::Format::eR8G8B8Unorm};
            vk::ColorSpaceKHR requestedColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
            for (size_t i = 0; i < sizeof(requestedFormats) / sizeof(requestedFormats[0]); i++) {
                vk::Format requestedFormat = requestedFormats[i];
                auto it = std::find_if(formats.begin(),
                                       formats.end(),
                                       [requestedFormat, requestedColorSpace](vk::SurfaceFormatKHR const& f) {
                                           return (f.format == requestedFormat) && (f.colorSpace == requestedColorSpace);
                                       });
                if (it != formats.end()) {
                    pickedFormat = *it;
                    break;
                }
            }
        }
        assert(pickedFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear);
        return pickedFormat;
    }

    void setImageLayout(
            vk::CommandBuffer const& commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldImageLayout,
            vk::ImageLayout newImageLayout, uint32_t levelCount, uint32_t layerCount, uint32_t baseArrayLayer, uint32_t baseMipLevel) {
        vk::AccessFlags sourceAccessMask;
        switch (oldImageLayout) {
            case vk::ImageLayout::eTransferDstOptimal:
                sourceAccessMask = vk::AccessFlagBits::eTransferWrite;
                break;
            case vk::ImageLayout::eTransferSrcOptimal:
                sourceAccessMask = vk::AccessFlagBits::eTransferRead;
                break;
            case vk::ImageLayout::ePreinitialized:
                sourceAccessMask = vk::AccessFlagBits::eHostWrite;
                break;
            case vk::ImageLayout::eGeneral:  // sourceAccessMask is empty
            case vk::ImageLayout::eUndefined:
                break;
            default:
                assert(false);
                break;
        }

        vk::PipelineStageFlags sourceStage;
        switch (oldImageLayout) {
            case vk::ImageLayout::eGeneral:
            case vk::ImageLayout::ePreinitialized:
                sourceStage = vk::PipelineStageFlagBits::eHost;
                break;
            case vk::ImageLayout::eTransferDstOptimal:
            case vk::ImageLayout::eTransferSrcOptimal:
                sourceStage = vk::PipelineStageFlagBits::eTransfer;
                break;
            case vk::ImageLayout::eUndefined:
                sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
                break;
            default:
                assert(false);
                break;
        }

        vk::AccessFlags destinationAccessMask;
        switch (newImageLayout) {
            case vk::ImageLayout::eColorAttachmentOptimal:
                destinationAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
                break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                destinationAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                break;
            case vk::ImageLayout::eGeneral:  // empty destinationAccessMask
            case vk::ImageLayout::ePresentSrcKHR:
                break;
            case vk::ImageLayout::eShaderReadOnlyOptimal:
                destinationAccessMask = vk::AccessFlagBits::eShaderRead;
                break;
            case vk::ImageLayout::eTransferSrcOptimal:
                destinationAccessMask = vk::AccessFlagBits::eTransferRead;
                break;
            case vk::ImageLayout::eTransferDstOptimal:
                destinationAccessMask = vk::AccessFlagBits::eTransferWrite;
                break;
            default:
                assert(false);
                break;
        }

        vk::PipelineStageFlags destinationStage;
        switch (newImageLayout) {
            case vk::ImageLayout::eColorAttachmentOptimal:
                destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
                break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
                break;
            case vk::ImageLayout::eGeneral:
                destinationStage = vk::PipelineStageFlagBits::eHost;
                break;
            case vk::ImageLayout::ePresentSrcKHR:
                destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;
                break;
            case vk::ImageLayout::eShaderReadOnlyOptimal:
                destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
                break;
            case vk::ImageLayout::eTransferDstOptimal:
            case vk::ImageLayout::eTransferSrcOptimal:
                destinationStage = vk::PipelineStageFlagBits::eTransfer;
                break;
            default:
                assert(false);
                break;
        }

        vk::ImageAspectFlags aspectMask;
        if (newImageLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            aspectMask = vk::ImageAspectFlagBits::eDepth;
            if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint) {
                aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        } else {
            aspectMask = vk::ImageAspectFlagBits::eColor;
        }

        vk::ImageSubresourceRange imageSubresourceRange(aspectMask, baseMipLevel, levelCount, baseArrayLayer, layerCount);
        vk::ImageMemoryBarrier imageMemoryBarrier(sourceAccessMask,
                                                  destinationAccessMask,
                                                  oldImageLayout,
                                                  newImageLayout,
                                                  VK_QUEUE_FAMILY_IGNORED,
                                                  VK_QUEUE_FAMILY_IGNORED,
                                                  image,
                                                  imageSubresourceRange);
        return commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, imageMemoryBarrier);
    }

    MonochromeImageGenerator::MonochromeImageGenerator(std::array<unsigned char, 3> const& rgb) : m_rgb(rgb) {}

    void MonochromeImageGenerator::operator()(void* data, vk::Extent2D const& extent) const {
        // fill in with the monochrome color
        auto* pImageMemory = static_cast<unsigned char*>( data );
        for (uint32_t row = 0; row < extent.height; row++) {
            for (uint32_t col = 0; col < extent.width; col++) {
                pImageMemory[0] = m_rgb[0];
                pImageMemory[1] = m_rgb[1];
                pImageMemory[2] = m_rgb[2];
                pImageMemory[3] = 255;
                pImageMemory += 4;
            }
        }
    }

    WindowData::WindowData(GLFWwindow* wnd, std::string name, vk::Extent2D const& extent) : handle{wnd}, name{std::move(name)}, extent{extent} {}

    WindowData::WindowData(WindowData&& other) noexcept
            : handle(other.handle),
              name(std::move(other.name)),
              extent(other.extent) {
    }

    WindowData& WindowData::operator=(WindowData&& other) noexcept {
        std::swap(handle, other.handle);
        std::swap(name, other.name);
        std::swap(extent, other.extent);
        return *this;
    }

    WindowData::~WindowData() noexcept {
        glfwDestroyWindow(handle);
    }

    WindowData createWindow(std::string const& windowName, vk::Extent2D const& extent) {
        struct glfwContext {
            glfwContext() {
                glfwInit();
                std::cout << "[GLFW] \"" << glfwGetVersionString() << "\" initialized" << std::endl;
                glfwSetErrorCallback(
                        [](int error, const char* msg) {
                            std::cerr << "glfw: "
                                      << "(" << error << ") " << msg << std::endl;
                        });
                if (!glfwVulkanSupported()) {
                    throw std::runtime_error("Vulkan is not supported");
                }
            }

            ~glfwContext() {
                glfwTerminate();
            }
        };

        static auto glfwCtx = glfwContext();
        (void) glfwCtx;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(static_cast<int>(extent.width), static_cast<int>(extent.height),
                                              windowName.c_str(), nullptr, nullptr);
        return {window, windowName, extent};
    }

    vk::DebugUtilsMessengerCreateInfoEXT makeDebugUtilsMessengerCreateInfoEXT() {
        return {{},
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
                &vk::su::debugUtilsMessengerCallback};
    }

#if defined( NDEBUG )
    vk::StructureChain<vk::InstanceCreateInfo>
#else

    vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
#endif
    makeInstanceCreateInfoChain(vk::ApplicationInfo const& applicationInfo,
                                std::vector<char const*> const& layers,
                                std::vector<char const*> const& extensions) {
#if defined( NDEBUG )
        // in non-debug mode just use the InstanceCreateInfo for instance creation
        vk::StructureChain<vk::InstanceCreateInfo> instanceCreateInfo( { {}, &applicationInfo, layers, extensions } );
#else
        // in debug mode, additionally use the debugUtilsMessengerCallback in instance creation!
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> instanceCreateInfo(
                {{}, &applicationInfo, layers, extensions}, {{}, severityFlags, messageTypeFlags, &vk::su::debugUtilsMessengerCallback});
#endif
        return instanceCreateInfo;
    }

}  // namespace vk

