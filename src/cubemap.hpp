#pragma once

#include "utils.hpp"

struct CubeMapImageData {
    CubeMapImageData(
            vk::PhysicalDevice const& physicalDevice,
            vk::Device const& device,
            vk::Format format,
            uint32_t dim,
            vk::ImageTiling tiling,
            vk::ImageUsageFlags usage,
            vk::ImageLayout initialLayout,
            vk::MemoryPropertyFlags memoryProperties,
            vk::ImageAspectFlags aspectMask
    ) : format(format) {
//        auto numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;
        uint32_t numMips = 1;
        vk::ImageCreateInfo imageCreateInfo(
                vk::ImageCreateFlagBits::eCubeCompatible,
                vk::ImageType::e2D,
                format,
                vk::Extent3D(dim, dim, 1),
                numMips,
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

        vk::ImageViewCreateInfo imageViewCreateInfo({}, image, vk::ImageViewType::eCube, format, {}, {aspectMask, 0, numMips, 0, 6});
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
            uint32_t dim = 256,
            vk::ImageUsageFlags usageFlags = {},
            vk::FormatFeatureFlags formatFeatureFlags = {},
            bool anisotropyEnable = false
    ) : format(vk::Format::eR8G8B8A8Unorm), dim(dim) {
        vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(format);
        formatFeatureFlags |= vk::FormatFeatureFlagBits::eSampledImage;
        vk::ImageTiling imageTiling;
        vk::ImageLayout initialLayout;
        vk::MemoryPropertyFlags requirements;
        assert((formatProperties.optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags);
        stagingBufferData = std::make_unique<vk::su::BufferData>(physicalDevice, device, dim * dim * 4, vk::BufferUsageFlagBits::eTransferSrc);
        imageTiling = vk::ImageTiling::eOptimal;
        usageFlags |= vk::ImageUsageFlagBits::eTransferDst;
        usageFlags |= vk::ImageUsageFlagBits::eTransferSrc;
        initialLayout = vk::ImageLayout::eUndefined;
        imageData = std::make_unique<CubeMapImageData>(
                physicalDevice,
                device,
                format,
                dim,
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
                vk::SamplerAddressMode::eClampToEdge,
                vk::SamplerAddressMode::eClampToEdge,
                vk::SamplerAddressMode::eClampToEdge,
                0.0f,
                anisotropyEnable,
                16.0f,
                false,
                vk::CompareOp::eNever,
                0.0f,
                static_cast<float>(floor(log2(dim)) + 1.0),
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
        vk::DeviceSize bufferSize = device.getBufferMemoryRequirements(stagingBufferData->buffer).size;
        void* data = device.mapMemory(stagingBufferData->deviceMemory, 0, bufferSize);
        vk::Extent2D extent(dim, dim);
        imageGenerator(data, extent);
        device.unmapMemory(stagingBufferData->deviceMemory);
        // Since we're going to blit to the texture image, set its layout to eTransferDstOptimal
        vk::su::setImageLayout(commandBuffer, imageData->image, imageData->format,
                               vk::ImageLayout::eUndefined, /*  ==>  */ vk::ImageLayout::eTransferDstOptimal,
                               1, 6);
        std::array<vk::BufferImageCopy, 6> copyRegions;
        for (size_t i = 0; i < 6; ++i) {
            copyRegions[i] = vk::BufferImageCopy(
                    0,
                    dim, dim,
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, i, 1),
                    vk::Offset3D(0, 0, 0),
                    vk::Extent3D(extent, 1)
            );
        }
        commandBuffer.copyBufferToImage(stagingBufferData->buffer, imageData->image, vk::ImageLayout::eTransferDstOptimal, copyRegions);

//        vk::su::setImageLayout(commandBuffer, imageData->image, imageData->format,
//                               vk::ImageLayout::eTransferDstOptimal, /*  ==>  */ vk::ImageLayout::eTransferSrcOptimal);
//        auto numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;
//        auto width = static_cast<int32_t>(extent.width);
//        auto height = static_cast<int32_t>(extent.height);
//        for (uint32_t mipLevel = 0; mipLevel < numMips; ++mipLevel) {
//            vk::ImageBlit imageBlit(
//                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mipLevel),
//                    {vk::Offset3D(0, 0, 1), vk::Offset3D(width, height, 1)},
//                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mipLevel + 1),
//                    {vk::Offset3D(0, 0, 1), vk::Offset3D(width / 2, height / 2, 1)}
//            );
//            commandBuffer.blitImage(imageData->image, vk::ImageLayout::eTransferSrcOptimal,
//                                    imageData->image, vk::ImageLayout::eTransferDstOptimal,
//                                    imageBlit, vk::Filter::eLinear);
//            width /= 2;
//            height /= 2;
//        }

        vk::su::setImageLayout(commandBuffer, imageData->image, imageData->format,
                               vk::ImageLayout::eTransferDstOptimal, /*  ==>  */ vk::ImageLayout::eShaderReadOnlyOptimal,
                               1, 6);
    }

    vk::Format format;
    uint32_t dim;
    std::unique_ptr<vk::su::BufferData> stagingBufferData;
    std::unique_ptr<CubeMapImageData> imageData;
    vk::Sampler sampler;
};

struct SkyboxImageGenerator {
    void operator()(void* data, vk::Extent2D& extent) const {
        auto* imageMemory = static_cast<uint8_t*>(data);
        memset(imageMemory, 255, extent.width * extent.height * 4);
//        for (uint32_t row = 0; row < extent.height; row++) {
//            for (uint32_t col = 0; col < extent.width; col++) {
//                for (uint32_t layer = 0; layer < 6; ++layer) {
//                    pImageMemory[0] = 255;
//                    pImageMemory[1] = 255;
//                    pImageMemory[2] = 255;
//                    pImageMemory[3] = 255;
//                    pImageMemory += 8;
//                }
//            }
//        }
    }
};
