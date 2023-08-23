//#pragma once
//
//#include "utils_raii.hpp"
//
//struct CubeMapData {
//    CubeMapData(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::Device const& device, uint32_t dim = 256)
//            : format(vk::Format::eR8G8B8A8Unorm),
//              dim(dim),
//              sampler(device, {
//                      {},
//                      vk::Filter::eLinear,
//                      vk::Filter::eLinear,
//                      vk::SamplerMipmapMode::eLinear,
//                      vk::SamplerAddressMode::eClampToEdge,
//                      vk::SamplerAddressMode::eClampToEdge,
//                      vk::SamplerAddressMode::eClampToEdge,
//                      0.0f,
//                      false,
//                      16.0f,
//                      false,
//                      vk::CompareOp::eNever,
//                      0.0f,
//                      static_cast<float>(floor(log2(dim)) + 1.0),
//                      vk::BorderColor::eFloatOpaqueBlack
//              }) {
//        vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(format);
//        stagingBufferData = vk::raii::su::BufferData(physicalDevice, device, dim * dim * 4, vk::BufferUsageFlagBits::eTransferSrc);
//        imageData = vk::raii::su::ImageData(
//                physicalDevice,
//                device,
//                format,
//                vk::Extent2D(dim, dim),
//                vk::ImageTiling::eOptimal,
//                vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
//                vk::ImageLayout::eUndefined,
//                vk::MemoryPropertyFlagBits::eDeviceLocal,
//                vk::ImageAspectFlagBits::eColor,
//                vk::ImageCreateFlagBits::eCubeCompatible,
//                vk::ImageViewType::eCube,
//                static_cast<uint32_t>(floor(log2(dim))) + 1,
//                6
//        );
//    }
//
//    template<typename ImageGenerator>
//    void setImage(vk::raii::Device const& device, vk::raii::CommandBuffer const& commandBuffer, ImageGenerator const& imageGenerator) {
//        vk::DeviceSize bufferSize = stagingBufferData->buffer->getMemoryRequirements().size;
//        void* data = stagingBufferData->deviceMemory->mapMemory(0, bufferSize);
//        vk::Extent2D extent(dim, dim);
//        imageGenerator(data, extent);
//        stagingBufferData->deviceMemory->unmapMemory();
//
//        // All images at all mip levels and layers start with destination optimal layout
//        auto numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;
//        vk::raii::su::setImageLayout(commandBuffer, **imageData->image, imageData->format,
//                                     vk::ImageLayout::eUndefined, /*  ==>  */ vk::ImageLayout::eTransferDstOptimal,
//                                     numMips, 6);
//        // Copy from staging buffer to the zeroth mip level of the image
//        // We will then use blit to generate the rest of the mip levels
//        std::array<vk::BufferImageCopy, 6> copyRegions;
//        for (uint32_t face = 0; face < 6; ++face) {
//            copyRegions[face] = vk::BufferImageCopy(
//                    0,
//                    dim, dim,
//                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, face, 1),
//                    vk::Offset3D(0, 0, 0),
//                    vk::Extent3D(extent, 1)
//            );
//        }
//        commandBuffer.copyBufferToImage(**stagingBufferData->buffer, **imageData->image, vk::ImageLayout::eTransferDstOptimal, copyRegions);
//
//        auto width = static_cast<int32_t>(extent.width);
//        auto height = static_cast<int32_t>(extent.height);
//        for (uint32_t face = 0; face < 6; ++face) {
//            for (uint32_t mipLevel = 1; mipLevel < numMips; ++mipLevel) {
//                // Blit the previous mip level (source) to the next mip level (destination)
//                vk::raii::su::setImageLayout(commandBuffer, **imageData->image, imageData->format,
//                                             vk::ImageLayout::eTransferDstOptimal, /*  ==>  */ vk::ImageLayout::eTransferSrcOptimal,
//                                             1, 1, face, mipLevel - 1);
//
//                vk::ImageBlit imageBlit(
//                        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mipLevel - 1, face, 1),
//                        {vk::Offset3D(0, 0, 0), vk::Offset3D(width, height, 1)},
//                        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mipLevel, face, 1),
//                        {vk::Offset3D(0, 0, 0), vk::Offset3D(std::max(1, width / 2), std::max(1, width / 2), 1)}
//                );
//                commandBuffer.blitImage(**imageData->image, vk::ImageLayout::eTransferSrcOptimal,
//                                        **imageData->image, vk::ImageLayout::eTransferDstOptimal,
//                                        imageBlit, vk::Filter::eLinear);
//                width = std::max(1, width / 2);
//                height = std::max(1, width / 2);
//
//                vk::raii::su::setImageLayout(commandBuffer, **imageData->image, imageData->format,
//                                             vk::ImageLayout::eTransferSrcOptimal, /*  ==>  */ vk::ImageLayout::eShaderReadOnlyOptimal,
//                                             1, 1, face, mipLevel - 1);
//            }
//
//            // Make sure to update the layout of the last mip level to read only
//            vk::raii::su::setImageLayout(commandBuffer, **imageData->image, imageData->format,
//                                         vk::ImageLayout::eTransferDstOptimal, /*  ==>  */ vk::ImageLayout::eShaderReadOnlyOptimal,
//                                         1, 1, face, numMips - 1);
//        }
//    }
//
//    vk::Format format;
//    uint32_t dim;
//    std::optional<vk::raii::su::BufferData> stagingBufferData;
//    std::optional<vk::raii::su::ImageData> imageData;
//    vk::raii::Sampler sampler;
//};
//
//struct SkyboxImageGenerator {
//    void operator()(void* data, vk::Extent2D& extent) const {
//        auto* imageMemory = static_cast<uint8_t*>(data);
//        memset(imageMemory, 255, extent.width * extent.height * 4);
////        for (uint32_t row = 0; row < extent.height; row++) {
////            for (uint32_t col = 0; col < extent.width; col++) {
////                for (uint32_t layer = 0; layer < 6; ++layer) {
////                    pImageMemory[0] = 255;
////                    pImageMemory[1] = 255;
////                    pImageMemory[2] = 255;
////                    pImageMemory[3] = 255;
////                    pImageMemory += 8;
////                }
////            }
////        }
//    }
//};
