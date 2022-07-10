#include "utils_raii.hpp"

vk::raii::su::SurfaceData::SurfaceData(vk::raii::Instance const& instance, std::string const& windowName, vk::Extent2D const& extent_)
        : extent(extent_), window(vk::su::createWindow(windowName, extent)) {
    VkSurfaceKHR _surface;
    VkResult err = glfwCreateWindowSurface(static_cast<VkInstance>( *instance ), window.handle, nullptr, &_surface);
    if (err != VK_SUCCESS)
        throw std::runtime_error("Failed to create window!");
    surface = vk::raii::SurfaceKHR(instance, _surface);
}

vk::raii::su::SwapChainData::SwapChainData(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::Device const& device,
                                           vk::raii::SurfaceKHR const& surface, vk::Extent2D const& extent, vk::ImageUsageFlags usage,
                                           vk::raii::SwapchainKHR const* pOldSwapchain, uint32_t graphicsQueueFamilyIndex,
                                           uint32_t presentQueueFamilyIndex) {
    vk::SurfaceFormatKHR surfaceFormat = vk::su::pickSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(*surface));
    colorFormat = surfaceFormat.format;

    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    vk::Extent2D swapchainExtent;
    if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
        // If the surface size is undefined, the size is set to the size of the images requested.
        swapchainExtent.width = std::clamp(extent.width, surfaceCapabilities.minImageExtent.width,
                                           surfaceCapabilities.maxImageExtent.width);
        swapchainExtent.height = std::clamp(extent.height, surfaceCapabilities.minImageExtent.height,
                                            surfaceCapabilities.maxImageExtent.height);
    } else {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfaceCapabilities.currentExtent;
    }
    vk::SurfaceTransformFlagBitsKHR preTransform = (surfaceCapabilities.supportedTransforms &
                                                    vk::SurfaceTransformFlagBitsKHR::eIdentity)
                                                   ? vk::SurfaceTransformFlagBitsKHR::eIdentity
                                                   : surfaceCapabilities.currentTransform;
    vk::CompositeAlphaFlagBitsKHR compositeAlpha =
            (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
            ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
            : (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
              ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
              : (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
                ? vk::CompositeAlphaFlagBitsKHR::eInherit
                : vk::CompositeAlphaFlagBitsKHR::eOpaque;
    vk::PresentModeKHR presentMode = vk::su::pickPresentMode(physicalDevice.getSurfacePresentModesKHR(*surface));
    vk::SwapchainCreateInfoKHR swapChainCreateInfo(
            {},
            *surface,
            surfaceCapabilities.minImageCount,
            colorFormat,
            surfaceFormat.colorSpace,
            swapchainExtent,
            1,
            usage,
            vk::SharingMode::eExclusive,
            {},
            preTransform,
            compositeAlpha,
            presentMode,
            true,
            pOldSwapchain ? **pOldSwapchain : nullptr
    );
    if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
        uint32_t queueFamilyIndices[2] = {graphicsQueueFamilyIndex, presentQueueFamilyIndex};
        // If the graphics and present queues are from different queue families, we either have to explicitly
        // transfer ownership of images between the queues, or we have to create the swapchain with imageSharingMode
        // as vk::SharingMode::eConcurrent
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);

    images = swapChain->getImages();

    imageViews.reserve(images.size());
    vk::ImageViewCreateInfo imageViewCreateInfo({}, {}, vk::ImageViewType::e2D, colorFormat, {},
                                                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    for (auto image: images) {
        imageViewCreateInfo.image = static_cast<vk::Image>( image );
        imageViews.emplace_back(device, imageViewCreateInfo);
    }
}

vk::raii::su::TextureData::TextureData(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::Device const& device, vk::Extent2D const& extent_,
                                       vk::ImageUsageFlags usageFlags, vk::FormatFeatureFlags formatFeatureFlags, bool anisotropyEnable,
                                       bool forceStaging)
        : format(vk::Format::eR8G8B8A8Srgb),
          extent(extent_),
          sampler(device,
                  {{},
                   vk::Filter::eLinear,
                   vk::Filter::eLinear,
                   vk::SamplerMipmapMode::eLinear,
                   vk::SamplerAddressMode::eRepeat,
                   vk::SamplerAddressMode::eRepeat,
                   vk::SamplerAddressMode::eRepeat,
                   0.0f,
                   anisotropyEnable,
                   16.0f,
                   false,
                   vk::CompareOp::eNever,
                   0.0f,
                   0.0f,
                   vk::BorderColor::eFloatOpaqueBlack}) {
    vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(format);

    formatFeatureFlags |= vk::FormatFeatureFlagBits::eSampledImage;
    needsStaging = forceStaging || ((formatProperties.linearTilingFeatures & formatFeatureFlags) != formatFeatureFlags);
    vk::ImageTiling imageTiling;
    vk::ImageLayout initialLayout;
    vk::MemoryPropertyFlags requirements;
    if (needsStaging) {
        GAME_ASSERT((formatProperties.optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags);
        stagingBufferData = BufferData(physicalDevice, device, extent.width * extent.height * 4,
                                       vk::BufferUsageFlagBits::eTransferSrc);
        imageTiling = vk::ImageTiling::eOptimal;
        usageFlags |= vk::ImageUsageFlagBits::eTransferDst;
        initialLayout = vk::ImageLayout::eUndefined;
    } else {
        imageTiling = vk::ImageTiling::eLinear;
        initialLayout = vk::ImageLayout::ePreinitialized;
        requirements = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible;
    }
    imageData = ImageData(
            physicalDevice,
            device,
            format,
            extent,
            imageTiling,
            usageFlags | vk::ImageUsageFlagBits::eSampled,
            initialLayout,
            requirements,
            vk::ImageAspectFlagBits::eColor
    );
}

vk::raii::su::ImageData::ImageData(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::Device const& device, vk::Format format_,
                                   vk::Extent2D const& extent, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::ImageLayout initialLayout,
                                   vk::MemoryPropertyFlags memoryProperties, vk::ImageAspectFlags aspectMask, vk::ImageCreateFlags imageFlags,
                                   vk::ImageViewType viewType,
                                   uint32_t mipLevel, uint32_t layerCount)
        : format(format_),
          image(vk::raii::Image(
                        device,
                        {imageFlags,
                         vk::ImageType::e2D,
                         format,
                         vk::Extent3D(extent, 1),
                         mipLevel,
                         layerCount,
                         vk::SampleCountFlagBits::e1,
                         tiling,
                         usage | vk::ImageUsageFlagBits::eSampled,
                         vk::SharingMode::eExclusive,
                         {},
                         initialLayout
                        }
                )
          ),
          deviceMemory(vk::raii::su::allocateDeviceMemory(
                  device, physicalDevice.getMemoryProperties(), image->getMemoryRequirements(), memoryProperties)) {
    image->bindMemory(**deviceMemory, 0);
    imageView = vk::raii::ImageView(device, vk::ImageViewCreateInfo({}, **image, viewType, format, {},
                                                                    {aspectMask, 0, layerCount, 0, layerCount}));
}

vk::raii::su::BufferData::BufferData(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::Device const& device, vk::DeviceSize size,
                                     vk::BufferUsageFlags usage, vk::MemoryPropertyFlags propertyFlags)
        : buffer(vk::raii::Buffer(device, vk::BufferCreateInfo({}, size, usage))),
          deviceMemory(vk::raii::su::allocateDeviceMemory(device, physicalDevice.getMemoryProperties(),
                                                          buffer->getMemoryRequirements(),
                                                          propertyFlags))
#if !defined( NDEBUG )
        , m_size(size), m_usage(usage), m_propertyFlags(propertyFlags)
#endif
{
    buffer->bindMemory(**deviceMemory, 0);
}

void vk::raii::su::setImageLayout(vk::raii::CommandBuffer const& commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldImageLayout,
                                  vk::ImageLayout newImageLayout,
                                  uint32_t levelCount, uint32_t layerCount, uint32_t baseArrayLevel, uint32_t baseMipLevel) {
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
            GAME_ASSERT(false);
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
            GAME_ASSERT(false);
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
            GAME_ASSERT(false);
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
            GAME_ASSERT(false);
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

    vk::ImageSubresourceRange imageSubresourceRange(aspectMask, baseMipLevel, levelCount, baseArrayLevel, layerCount);
    vk::ImageMemoryBarrier imageMemoryBarrier(
            sourceAccessMask,
            destinationAccessMask,
            oldImageLayout,
            newImageLayout,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            image,
            imageSubresourceRange
    );
    return commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, imageMemoryBarrier);
}

vk::raii::DeviceMemory vk::raii::su::allocateDeviceMemory(vk::raii::Device const& device, vk::PhysicalDeviceMemoryProperties const& memoryProperties,
                                                          vk::MemoryRequirements const& memoryRequirements,
                                                          vk::MemoryPropertyFlags memoryPropertyFlags) {
    uint32_t memoryTypeIndex = vk::su::findMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryPropertyFlags);
    vk::MemoryAllocateInfo memoryAllocateInfo(memoryRequirements.size, memoryTypeIndex);
    return {device, memoryAllocateInfo};
}

vk::raii::su::DepthBufferData::DepthBufferData(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::Device const& device, vk::Format format,
                                               vk::Extent2D const& extent)
        : ImageData(physicalDevice,
                    device,
                    format,
                    extent,
                    vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eDepthStencilAttachment,
                    vk::ImageLayout::eUndefined,
                    vk::MemoryPropertyFlagBits::eDeviceLocal,
                    vk::ImageAspectFlagBits::eDepth) {
}

std::pair<uint32_t, uint32_t>
vk::raii::su::findGraphicsAndPresentQueueFamilyIndex(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::SurfaceKHR const& surface) {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    GAME_ASSERT(queueFamilyProperties.size() < std::numeric_limits<uint32_t>::max());

    uint32_t graphicsQueueFamilyIndex = vk::su::findGraphicsQueueFamilyIndex(queueFamilyProperties);
    if (physicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, *surface)) {
        return std::make_pair(graphicsQueueFamilyIndex,
                              graphicsQueueFamilyIndex);  // the first graphicsQueueFamilyIndex does also support presents
    }

    // the graphicsQueueFamilyIndex doesn't support present -> look for an other family index that supports both
    // graphics and present
    for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
        if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>( i ), *surface)) {
            return std::make_pair(static_cast<uint32_t>( i ), static_cast<uint32_t>( i ));
        }
    }

    // there's nothing like a single family index that supports both graphics and present -> look for an other
    // family index that supports present
    for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
        if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>( i ), *surface)) {
            return std::make_pair(graphicsQueueFamilyIndex, static_cast<uint32_t>( i ));
        }
    }

    throw std::runtime_error("Could not find queues for both graphics or present -> terminating");
}

vk::raii::DescriptorPool vk::raii::su::makeDescriptorPool(vk::raii::Device const& device, std::vector<vk::DescriptorPoolSize> const& poolSizes) {
    GAME_ASSERT(!poolSizes.empty());
    uint32_t maxSets = std::accumulate(
            poolSizes.begin(), poolSizes.end(), 0,
            [](uint32_t sum, vk::DescriptorPoolSize const& dps) { return sum + dps.descriptorCount; });
    GAME_ASSERT(0 < maxSets);

    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, maxSets, poolSizes);
    return {device, descriptorPoolCreateInfo};
}

vk::raii::Device
vk::raii::su::makeDevice(vk::raii::PhysicalDevice const& physicalDevice, uint32_t queueFamilyIndex, std::vector<std::string> const& extensions,
                         vk::PhysicalDeviceFeatures const* physicalDeviceFeatures, void const* pNext) {
    std::vector<char const*> enabledExtensions;
    enabledExtensions.reserve(extensions.size());
    for (auto const& ext: extensions) {
        enabledExtensions.push_back(ext.data());
    }

    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), queueFamilyIndex, 1, &queuePriority);
    vk::DeviceCreateInfo deviceCreateInfo(vk::DeviceCreateFlags(), deviceQueueCreateInfo, {}, enabledExtensions, physicalDeviceFeatures);
    return {physicalDevice, deviceCreateInfo};
}

vk::raii::Pipeline vk::raii::su::makeGraphicsPipeline(vk::raii::Device const& device, vk::raii::PipelineCache const& pipelineCache,
                                                      vk::raii::ShaderModule const& vertexShaderModule,
                                                      vk::SpecializationInfo const* vertexShaderSpecializationInfo,
                                                      vk::raii::ShaderModule const& fragmentShaderModule,
                                                      vk::SpecializationInfo const* fragmentShaderSpecializationInfo, uint32_t vertexStride,
                                                      std::vector<std::pair<vk::Format, uint32_t>> const& vertexInputAttributeFormatOffset,
                                                      vk::FrontFace frontFace, bool depthBuffered, vk::raii::PipelineLayout const& pipelineLayout,
                                                      vk::raii::RenderPass const& renderPass) {
    std::array<vk::PipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfos = {
            vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vertexShaderModule, "main",
                                              vertexShaderSpecializationInfo),
            vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *fragmentShaderModule, "main",
                                              fragmentShaderSpecializationInfo)
    };

    std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions;
    vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
    vk::VertexInputBindingDescription vertexInputBindingDescription(0, vertexStride);

    if (0 < vertexStride) {
        vertexInputAttributeDescriptions.reserve(vertexInputAttributeFormatOffset.size());
        for (uint32_t i = 0; i < vertexInputAttributeFormatOffset.size(); i++) {
            vertexInputAttributeDescriptions.emplace_back(i, 0, vertexInputAttributeFormatOffset[i].first,
                                                          vertexInputAttributeFormatOffset[i].second);
        }
        pipelineVertexInputStateCreateInfo.setVertexBindingDescriptions(vertexInputBindingDescription);
        pipelineVertexInputStateCreateInfo.setVertexAttributeDescriptions(vertexInputAttributeDescriptions);
    }

    vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(vk::PipelineInputAssemblyStateCreateFlags(),
                                                                                  vk::PrimitiveTopology::eTriangleList);

    vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(vk::PipelineViewportStateCreateFlags(), 1, nullptr, 1, nullptr);

    vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
            vk::PipelineRasterizationStateCreateFlags(),
            false,
            false,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            frontFace,
            false,
            0.0f,
            0.0f,
            0.0f,
            1.0f
    );

    vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1);

    vk::StencilOpState stencilOpState(vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways);
    vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
            vk::PipelineDepthStencilStateCreateFlags(), depthBuffered, depthBuffered, vk::CompareOp::eLessOrEqual, false, false,
            stencilOpState, stencilOpState);

    vk::ColorComponentFlags colorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
                                                vk::ColorComponentFlagBits::eA);
    vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(false,
                                                                            vk::BlendFactor::eZero,
                                                                            vk::BlendFactor::eZero,
                                                                            vk::BlendOp::eAdd,
                                                                            vk::BlendFactor::eZero,
                                                                            vk::BlendFactor::eZero,
                                                                            vk::BlendOp::eAdd,
                                                                            colorComponentFlags);
    vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
            vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eNoOp, pipelineColorBlendAttachmentState,
            {{1.0f, 1.0f, 1.0f, 1.0f}});

    std::array<vk::DynamicState, 2> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(vk::PipelineDynamicStateCreateFlags(), dynamicStates);

    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
            vk::PipelineCreateFlags(),
            pipelineShaderStageCreateInfos,
            &pipelineVertexInputStateCreateInfo,
            &pipelineInputAssemblyStateCreateInfo,
            nullptr,
            &pipelineViewportStateCreateInfo,
            &pipelineRasterizationStateCreateInfo,
            &pipelineMultisampleStateCreateInfo,
            &pipelineDepthStencilStateCreateInfo,
            &pipelineColorBlendStateCreateInfo,
            &pipelineDynamicStateCreateInfo,
            *pipelineLayout,
            *renderPass
    );

    return {device, pipelineCache, graphicsPipelineCreateInfo};
}

vk::raii::RenderPass
vk::raii::su::makeRenderPass(vk::raii::Device const& device, vk::Format colorFormat, vk::Format depthFormat, vk::AttachmentLoadOp loadOp,
                             vk::ImageLayout colorFinalLayout) {
    std::vector<vk::AttachmentDescription> attachmentDescriptions;
    GAME_ASSERT(colorFormat != vk::Format::eUndefined);
    attachmentDescriptions.emplace_back(
            vk::AttachmentDescriptionFlags(),
            colorFormat,
            vk::SampleCountFlagBits::e1,
            loadOp,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            colorFinalLayout
    );
    if (depthFormat != vk::Format::eUndefined) {
        attachmentDescriptions.emplace_back(
                vk::AttachmentDescriptionFlags(),
                depthFormat,
                vk::SampleCountFlagBits::e1,
                loadOp,
                vk::AttachmentStoreOp::eDontCare,
                vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eDepthStencilAttachmentOptimal
        );
    }
    vk::AttachmentReference colorAttachment(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthAttachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::SubpassDescription subpassDescription(
            vk::SubpassDescriptionFlags(),
            vk::PipelineBindPoint::eGraphics,
            {},
            colorAttachment,
            {},
            (depthFormat != vk::Format::eUndefined) ? &depthAttachment : nullptr
    );
    vk::RenderPassCreateInfo renderPassCreateInfo(vk::RenderPassCreateFlags(), attachmentDescriptions, subpassDescription);
    return {device, renderPassCreateInfo};
}

vk::raii::Instance vk::raii::su::makeInstance(vk::raii::Context const& context, std::string const& appName, std::string const& engineName,
                                              std::vector<std::string> const& layers, std::vector<std::string> const& extensions,
                                              uint32_t apiVersion) {
    vk::ApplicationInfo applicationInfo(appName.c_str(), 1, engineName.c_str(), 1, apiVersion);
    std::vector<char const*> enabledLayers = vk::su::gatherLayers(layers
#if !defined( NDEBUG )
            , context.enumerateInstanceLayerProperties()
#endif
    );
    std::vector<char const*> enabledExtensions = vk::su::gatherExtensions(extensions
#if !defined( NDEBUG )
            , context.enumerateInstanceExtensionProperties()
#endif
    );
#if defined( NDEBUG )
    vk::StructureChain<vk::InstanceCreateInfo>
#else
    vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
#endif
            instanceCreateInfoChain = vk::su::makeInstanceCreateInfoChain(applicationInfo, enabledLayers, enabledExtensions);

    return {context, instanceCreateInfoChain.get<vk::InstanceCreateInfo>()};
}

vk::Format vk::raii::su::pickDepthFormat(vk::raii::PhysicalDevice const& physicalDevice) {
    std::vector<vk::Format> candidates = {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint};
    for (vk::Format format: candidates) {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);

        if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

vk::raii::Image vk::raii::su::makeImage(vk::raii::Device const& device) {
    vk::ImageCreateInfo imageCreateInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eB8G8R8A8Unorm,
            vk::Extent3D(640, 640, 1),
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eLinear,
            vk::ImageUsageFlagBits::eTransferSrc
    );
    return {device, imageCreateInfo};
}

std::vector<vk::raii::Framebuffer>
vk::raii::su::makeFramebuffers(vk::raii::Device const& device, vk::raii::RenderPass& renderPass, std::vector<vk::raii::ImageView> const& imageViews,
                               vk::raii::ImageView const* pDepthImageView, vk::Extent2D const& extent) {
    vk::ImageView attachments[2];
    attachments[1] = pDepthImageView ? **pDepthImageView : vk::ImageView();

    vk::FramebufferCreateInfo framebufferCreateInfo(vk::FramebufferCreateFlags(), *renderPass, pDepthImageView ? 2 : 1, attachments, extent.width,
                                                    extent.height, 1);
    std::vector<vk::raii::Framebuffer> framebuffers;
    framebuffers.reserve(imageViews.size());
    for (auto const& imageView: imageViews) {
        attachments[0] = *imageView;
        framebuffers.emplace_back(device, framebufferCreateInfo);
    }

    return framebuffers;
}

void vk::raii::su::submitAndWait(vk::raii::Device const& device, vk::raii::Queue const& queue, vk::raii::CommandBuffer const& commandBuffer) {
    vk::raii::Fence fence(device, vk::FenceCreateInfo());
    queue.submit(vk::SubmitInfo(nullptr, nullptr, *commandBuffer), *fence);
    while (vk::Result::eTimeout == device.waitForFences({*fence}, VK_TRUE, vk::su::FenceTimeout));
}
