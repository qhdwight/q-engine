#pragma once

#include "utils.hpp"

struct CubeImageData {
    CubeImageData(
            vk::PhysicalDevice const& physicalDevice,
            vk::Device const& device,
            vk::Format format,
            vk::Extent3D const& extent,
            vk::ImageTiling tiling,
            vk::ImageUsageFlags usage,
            vk::ImageLayout initialLayout,
            vk::MemoryPropertyFlags memoryProperties,
            vk::ImageAspectFlags aspectMask
    ) : format(format) {
        vk::ImageCreateInfo imageCreateInfo(
                vk::ImageCreateFlagBits::eCubeCompatible,
                vk::ImageType::e2D,
                format,
                extent,
                1,
                6,
                vk::SampleCountFlagBits::e1,
                tiling,
                usage | vk::ImageUsageFlagBits::eSampled,
                vk::SharingMode::eExclusive,
                {},
                initialLayout
        );
        image = device.createImage(imageCreateInfo);

        deviceMemory = vk::su::allocateDeviceMemory(device, physicalDevice.getMemoryProperties(), device.getImageMemoryRequirements(image),
                                                    memoryProperties);

        device.bindImageMemory(image, deviceMemory, 0);

        vk::ImageViewCreateInfo imageViewCreateInfo({}, image, vk::ImageViewType::e2D, format, {}, {aspectMask, 0, 1, 0, 1});
        imageView = device.createImageView(imageViewCreateInfo);
    }

    void clear(vk::Device const& device) const {
        device.destroyImageView(imageView);
        device.freeMemory(deviceMemory);
        device.destroyImage(image);
    }

    vk::Format format;
    vk::Image image;
    vk::DeviceMemory deviceMemory;
    vk::ImageView imageView;
};

struct CubeMapData {
    CubeMapData(
            vk::PhysicalDevice const& physicalDevice,
            vk::Device const& device,
            vk::Extent3D const& extent = {256, 256, 1},
            vk::ImageUsageFlags usageFlags = {},
            vk::FormatFeatureFlags formatFeatureFlags = {},
            bool anisotropyEnable = false,
            bool forceStaging = false
    ) : format(vk::Format::eR8G8B8A8Unorm), extent(extent) {
        vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(format);

        formatFeatureFlags |= vk::FormatFeatureFlagBits::eSampledImage;
        needsStaging = forceStaging || ((formatProperties.linearTilingFeatures & formatFeatureFlags) != formatFeatureFlags);
        vk::ImageTiling imageTiling;
        vk::ImageLayout initialLayout;
        vk::MemoryPropertyFlags requirements;
        if (needsStaging) {
            assert((formatProperties.optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags);
            stagingBufferData = std::make_unique<vk::su::BufferData>
                    (physicalDevice, device, extent.width * extent.height * 4, vk::BufferUsageFlagBits::eTransferSrc);
            imageTiling = vk::ImageTiling::eOptimal;
            usageFlags |= vk::ImageUsageFlagBits::eTransferDst;
            initialLayout = vk::ImageLayout::eUndefined;
        } else {
            imageTiling = vk::ImageTiling::eLinear;
            initialLayout = vk::ImageLayout::ePreinitialized;
            requirements = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible;
        }
        imageData = std::make_unique<CubeImageData>(
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

        sampler = device.createSampler(vk::SamplerCreateInfo(
                vk::SamplerCreateFlags(),
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
                vk::BorderColor::eFloatOpaqueBlack
        ));
    }

    void clear(vk::Device const& device) const {
        if (stagingBufferData) {
            stagingBufferData->clear(device);
        }
        imageData->clear(device);
        device.destroySampler(sampler);
    }

    template<typename ImageGenerator>
    void setImage(vk::Device const& device, vk::CommandBuffer const& commandBuffer, ImageGenerator const& imageGenerator) {
        void* data = needsStaging
                     ? device.mapMemory(stagingBufferData->deviceMemory, 0,
                                        device.getBufferMemoryRequirements(stagingBufferData->buffer).size)
                     : device.mapMemory(imageData->deviceMemory, 0, device.getImageMemoryRequirements(imageData->image).size);
        imageGenerator(data, extent);
        device.unmapMemory(needsStaging ? stagingBufferData->deviceMemory : imageData->deviceMemory);

        if (needsStaging) {
            // Since we're going to blit to the texture image, set its layout to eTransferDstOptimal
            vk::su::setImageLayout(commandBuffer, imageData->image, imageData->format, vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eTransferDstOptimal);
            vk::BufferImageCopy copyRegion(
                    0,
                    extent.width,
                    extent.height,
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 6),
                    vk::Offset3D(0, 0, 0),
                    extent
            );
            commandBuffer.copyBufferToImage(stagingBufferData->buffer, imageData->image, vk::ImageLayout::eTransferDstOptimal, copyRegion);
            // Set the layout for the texture image from eTransferDstOptimal to SHADER_READ_ONLY
            vk::su::setImageLayout(commandBuffer, imageData->image, imageData->format, vk::ImageLayout::eTransferDstOptimal,
                                   vk::ImageLayout::eShaderReadOnlyOptimal);
        } else {
            // If we can use the linear tiled image as a texture, just do it
            vk::su::setImageLayout(commandBuffer, imageData->image, imageData->format, vk::ImageLayout::ePreinitialized,
                                   vk::ImageLayout::eShaderReadOnlyOptimal);
        }
    }

    vk::Format format;
    vk::Extent3D extent;
    bool needsStaging;
    std::unique_ptr<vk::su::BufferData> stagingBufferData;
    std::unique_ptr<CubeImageData> imageData;
    vk::Sampler sampler;
};

struct CheckerboardImageGenerator {
private:
    std::array<uint8_t, 3> const& m_rgb0;
    std::array<uint8_t, 3> const& m_rgb1;

public:
    explicit CheckerboardImageGenerator(std::array<uint8_t, 3> const& rgb0 = {{0, 0, 0}}, std::array<uint8_t, 3> const& rgb1 = {{255, 255, 255}})
            : m_rgb0(rgb0), m_rgb1(rgb1) {
    }

    void operator()(void* data, vk::Extent3D& extent) const {
        auto* pImageMemory = static_cast<uint8_t*>( data );
        for (uint32_t row = 0; row < extent.height; row++) {
            for (uint32_t col = 0; col < extent.width; col++) {
                std::array<uint8_t, 3> const& rgb = (((row & 0x10) == 0) ^ ((col & 0x10) == 0)) ? m_rgb1 : m_rgb0;
                pImageMemory[0] = rgb[0];
                pImageMemory[1] = rgb[1];
                pImageMemory[2] = rgb[2];
                pImageMemory[3] = 255;
                pImageMemory += 4;
            }
        }
    }
};
