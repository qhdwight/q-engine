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

#include <numeric>
#include <optional>

#include <vulkan/vulkan_raii.hpp>

#include "utils.hpp"

namespace vk::raii::su {
    vk::raii::DeviceMemory allocateDeviceMemory(vk::raii::Device const& device,
                                                vk::PhysicalDeviceMemoryProperties const& memoryProperties,
                                                vk::MemoryRequirements const& memoryRequirements,
                                                vk::MemoryPropertyFlags memoryPropertyFlags);

    template<typename T>
    void copyToDevice(vk::raii::DeviceMemory const& deviceMemory, T const* pData, size_t count, vk::DeviceSize stride = sizeof(T)) {
        assert(sizeof(T) <= stride);
        uint8_t* deviceData = static_cast<uint8_t*>( deviceMemory.mapMemory(0, count * stride));
        if (stride == sizeof(T)) {
            memcpy(deviceData, pData, count * sizeof(T));
        } else {
            for (size_t i = 0; i < count; i++) {
                memcpy(deviceData, &pData[i], sizeof(T));
                deviceData += stride;
            }
        }
        deviceMemory.unmapMemory();
    }

    template<typename T>
    void copyToDevice(vk::raii::DeviceMemory const& deviceMemory, T const& data) {
        copyToDevice<T>(deviceMemory, &data, 1);
    }

    template<typename Func>
    void oneTimeSubmit(vk::raii::CommandBuffer const& commandBuffer, vk::raii::Queue const& queue, Func const& func) {
        commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        func(commandBuffer);
        commandBuffer.end();
        vk::SubmitInfo submitInfo(nullptr, nullptr, *commandBuffer);
        queue.submit(submitInfo, nullptr);
        queue.waitIdle();
    }

    template<typename Func>
    void oneTimeSubmit(vk::raii::Device const& device, vk::raii::CommandPool const& commandPool, vk::raii::Queue const& queue, Func const& func) {
        vk::raii::CommandBuffers commandBuffers(device, {*commandPool, vk::CommandBufferLevel::ePrimary, 1});
        oneTimeSubmit(commandBuffers.front(), queue, func);
    }

    void setImageLayout(vk::raii::CommandBuffer const& commandBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldImageLayout,
                        vk::ImageLayout newImageLayout,
                        uint32_t levelCount = 1, uint32_t layerCount = 1, uint32_t baseArrayLevel = 0, uint32_t baseMipLevel = 0);

    struct BufferData {
        BufferData(vk::raii::PhysicalDevice const& physicalDevice,
                   vk::raii::Device const& device,
                   vk::DeviceSize size,
                   vk::BufferUsageFlags usage,
                   vk::MemoryPropertyFlags propertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        template<typename DataType>
        void upload(DataType const& data) const {
            assert((m_propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent) &&
                   (m_propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible));
            assert(sizeof(DataType) <= m_size);

            void* dataPtr = deviceMemory->mapMemory(0, sizeof(DataType));
            memcpy(dataPtr, &data, sizeof(DataType));
            deviceMemory->unmapMemory();
        }

        template<typename DataType>
        void upload(std::vector<DataType> const& data, size_t stride = 0) const {
            assert(m_propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible);

            size_t elementSize = stride ? stride : sizeof(DataType);
            assert(sizeof(DataType) <= elementSize);

            copyToDevice(deviceMemory, data.data(), data.size(), elementSize);
        }

        template<typename DataType>
        void upload(vk::raii::PhysicalDevice const& physicalDevice,
                    vk::raii::Device const& device,
                    vk::raii::CommandPool const& commandPool,
                    vk::raii::Queue const& queue,
                    std::vector<DataType> const& data,
                    size_t stride) const {
            assert(m_usage & vk::BufferUsageFlagBits::eTransferDst);
            assert(m_propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal);

            size_t elementSize = stride ? stride : sizeof(DataType);
            assert(sizeof(DataType) <= elementSize);

            size_t dataSize = data.size() * elementSize;
            assert(dataSize <= m_size);

            vk::raii::su::BufferData stagingBuffer(physicalDevice, device, dataSize, vk::BufferUsageFlagBits::eTransferSrc);
            copyToDevice(stagingBuffer.deviceMemory, data.data(), data.size(), elementSize);

            vk::raii::su::oneTimeSubmit(device, commandPool, queue,
                                        [&](vk::raii::CommandBuffer const& commandBuffer) {
                                            commandBuffer.copyBuffer(**stagingBuffer.buffer, **this->buffer, vk::BufferCopy(0, 0, dataSize));
                                        });
        }

        // the order of buffer and deviceMemory here is important to get the constructor running !
        std::optional<vk::raii::Buffer> buffer;
        std::optional<vk::raii::DeviceMemory> deviceMemory;
#if !defined( NDEBUG )
    private:
        vk::DeviceSize m_size;
        vk::BufferUsageFlags m_usage;
        vk::MemoryPropertyFlags m_propertyFlags;
#endif
    };

    struct ImageData {
        ImageData(vk::raii::PhysicalDevice const& physicalDevice,
                  vk::raii::Device const& device,
                  vk::Format format_,
                  vk::Extent2D const& extent,
                  vk::ImageTiling tiling,
                  vk::ImageUsageFlags usage,
                  vk::ImageLayout initialLayout,
                  vk::MemoryPropertyFlags memoryProperties,
                  vk::ImageAspectFlags aspectMask,
                  vk::ImageCreateFlags imageFlags = {},
                  vk::ImageViewType viewType = vk::ImageViewType::e2D,
                  uint32_t mipLevel = 1, uint32_t layerCount = 1);

        vk::Format format;
        std::optional<vk::raii::Image> image;
        std::optional<vk::raii::DeviceMemory> deviceMemory;
        std::optional<vk::raii::ImageView> imageView;
    };

    struct DepthBufferData : public ImageData {
        DepthBufferData(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::Device const& device, vk::Format format,
                        vk::Extent2D const& extent);
    };

    struct SurfaceData {
        SurfaceData(vk::raii::Instance const& instance, std::string const& windowName, vk::Extent2D const& extent_);

        vk::Extent2D extent;
        vk::su::WindowData window;
        std::optional<vk::raii::SurfaceKHR> surface;
    };

    struct SwapChainData {
        SwapChainData(vk::raii::PhysicalDevice const& physicalDevice,
                      vk::raii::Device const& device,
                      vk::raii::SurfaceKHR const& surface,
                      vk::Extent2D const& extent,
                      vk::ImageUsageFlags usage,
                      vk::raii::SwapchainKHR const* pOldSwapchain,
                      uint32_t graphicsQueueFamilyIndex,
                      uint32_t presentQueueFamilyIndex);

        vk::Format colorFormat;
        std::optional<vk::raii::SwapchainKHR> swapChain;
        std::vector<VkImage> images;
        std::vector<vk::raii::ImageView> imageViews;
    };

    struct TextureData {
        TextureData(vk::raii::PhysicalDevice const& physicalDevice,
                    vk::raii::Device const& device,
                    vk::Extent2D const& extent_ = {256, 256},
                    vk::ImageUsageFlags usageFlags = {},
                    vk::FormatFeatureFlags formatFeatureFlags = {},
                    bool anisotropyEnable = false,
                    bool forceStaging = false);

        template<typename ImageGenerator>
        void setImage(vk::raii::CommandBuffer const& commandBuffer, ImageGenerator const& imageGenerator) {
            void* data = needsStaging ? stagingBufferData->deviceMemory->mapMemory(0, stagingBufferData->buffer->getMemoryRequirements().size)
                                      : imageData->deviceMemory->mapMemory(0, imageData->image->getMemoryRequirements().size);
            imageGenerator(data, extent);
            needsStaging ? stagingBufferData->deviceMemory->unmapMemory() : imageData->deviceMemory->unmapMemory();

            if (needsStaging) {
                // Since we're going to blit to the texture image, set its layout to eTransferDstOptimal
                vk::raii::su::setImageLayout(commandBuffer, **imageData->image, imageData->format, vk::ImageLayout::eUndefined,
                                             vk::ImageLayout::eTransferDstOptimal);
                vk::BufferImageCopy copyRegion(
                        0,
                        extent.width,
                        extent.height,
                        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                        vk::Offset3D(0, 0, 0),
                        vk::Extent3D(extent, 1)
                );
                commandBuffer.copyBufferToImage(**stagingBufferData->buffer, **imageData->image, vk::ImageLayout::eTransferDstOptimal, copyRegion);
                // Set the layout for the texture image from eTransferDstOptimal to eShaderReadOnlyOptimal
                vk::raii::su::setImageLayout(commandBuffer, **imageData->image, imageData->format, vk::ImageLayout::eTransferDstOptimal,
                                             vk::ImageLayout::eShaderReadOnlyOptimal);
            } else {
                // If we can use the linear tiled image as a texture, just do it
                vk::raii::su::setImageLayout(commandBuffer, **imageData->image, imageData->format, vk::ImageLayout::ePreinitialized,
                                             vk::ImageLayout::eShaderReadOnlyOptimal);
            }
        }

        vk::Format format;
        vk::Extent2D extent;
        bool needsStaging;
        std::optional<BufferData> stagingBufferData;
        std::optional<ImageData> imageData;
        vk::raii::Sampler sampler;
    };

    std::pair<uint32_t, uint32_t> findGraphicsAndPresentQueueFamilyIndex(vk::raii::PhysicalDevice const& physicalDevice,
                                                                         vk::raii::SurfaceKHR const& surface);

    vk::raii::DescriptorPool makeDescriptorPool(vk::raii::Device const& device, std::vector<vk::DescriptorPoolSize> const& poolSizes);

    vk::raii::Device makeDevice(vk::raii::PhysicalDevice const& physicalDevice,
                                uint32_t queueFamilyIndex,
                                std::vector<std::string> const& extensions = {},
                                vk::PhysicalDeviceFeatures const* physicalDeviceFeatures = nullptr,
                                void const* pNext = nullptr);

    std::vector<vk::raii::Framebuffer> makeFramebuffers(vk::raii::Device const& device,
                                                        vk::raii::RenderPass& renderPass,
                                                        std::vector<vk::raii::ImageView> const& imageViews,
                                                        vk::raii::ImageView const* pDepthImageView,
                                                        vk::Extent2D const& extent);

    vk::raii::Pipeline makeGraphicsPipeline(vk::raii::Device const& device,
                                            vk::raii::PipelineCache const& pipelineCache,
                                            vk::raii::ShaderModule const& vertexShaderModule,
                                            vk::SpecializationInfo const* vertexShaderSpecializationInfo,
                                            vk::raii::ShaderModule const& fragmentShaderModule,
                                            vk::SpecializationInfo const* fragmentShaderSpecializationInfo,
                                            uint32_t vertexStride,
                                            std::vector<std::pair<vk::Format, uint32_t>> const& vertexInputAttributeFormatOffset,
                                            vk::FrontFace frontFace,
                                            bool depthBuffered,
                                            vk::raii::PipelineLayout const& pipelineLayout,
                                            vk::raii::RenderPass const& renderPass);

    vk::raii::Image makeImage(vk::raii::Device const& device);

    vk::raii::Instance makeInstance(vk::raii::Context const& context,
                                    std::string const& appName,
                                    std::string const& engineName,
                                    std::vector<std::string> const& layers = {},
                                    std::vector<std::string> const& extensions = {},
                                    uint32_t apiVersion = VK_API_VERSION_1_0);

    vk::raii::RenderPass makeRenderPass(vk::raii::Device const& device,
                                        vk::Format colorFormat,
                                        vk::Format depthFormat,
                                        vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear,
                                        vk::ImageLayout colorFinalLayout = vk::ImageLayout::ePresentSrcKHR);

    vk::Format pickDepthFormat(vk::raii::PhysicalDevice const& physicalDevice);

    void submitAndWait(vk::raii::Device const& device, vk::raii::Queue const& queue, vk::raii::CommandBuffer const& commandBuffer);
}
