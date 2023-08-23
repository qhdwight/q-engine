module;

#include <pch.hpp>

export module render:memory;

import vulkan;

constexpr vk::Format DEPTH_FORMAT = vk::Format::eD32Sfloat;

export struct MemAllocator : std::shared_ptr<VmaAllocator> {

    MemAllocator() = default;

    MemAllocator(vk::raii::Instance const& instance, vk::raii::Device const& device, vk::raii::PhysicalDevice const& physicalDevice) {
        VmaVulkanFunctions functions{
                .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
                .vkGetDeviceProcAddr = &vkGetDeviceProcAddr,
        };

        VmaAllocatorCreateInfo create_info{
                .physicalDevice = static_cast<VkPhysicalDevice>(*physicalDevice),
                .device = static_cast<VkDevice>(*device),
                .pVulkanFunctions = &functions,
                .instance = static_cast<VkInstance>(*instance),
                .vulkanApiVersion = VK_API_VERSION_1_3,
        };

        auto* raw = new VmaAllocator{};
        if (vmaCreateAllocator(&create_info, raw) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create memory allocator");
        }
        reset(raw, [](VmaAllocator* p) {
            vmaDestroyAllocator(*p);
            delete p;
        });
    }
};

export struct Image {

    MemAllocator allocator;
    vk::Image image;
    vk::raii::ImageView view = nullptr;
    VmaAllocation allocation{};

    Image(const Image&) = delete;

    Image& operator=(Image&) = delete;

    Image(Image&&) = default;

    Image& operator=(Image&&) = default;

    Image(MemAllocator const& allocator, VmaAllocationCreateInfo const& allocInfo, vk::ImageCreateInfo const& imageInfo)
            : allocator{allocator} {

        auto createInfoRaw = static_cast<VkImageCreateInfo>(imageInfo);
        VkImage imageRaw;
        vmaCreateImage(*allocator, &createInfoRaw, &allocInfo, &imageRaw, &allocation, nullptr);
        image = imageRaw;
    }

    ~Image() {
        if (!allocator || !image) return;

        vmaDestroyImage(*allocator, static_cast<VkImage>(image), allocation);
    }
};

export struct DepthImage : Image {

    explicit DepthImage(vk::raii::Device const& device, MemAllocator const& allocator, vk::Extent2D const& extent)
            : Image{
            allocator,
            VmaAllocationCreateInfo{
                    .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                    .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            },
            vk::ImageCreateInfo{
                    {},
                    vk::ImageType::e2D,
                    vk::Format::eD32Sfloat,
                    vk::Extent3D{extent, 1},
                    1,
                    1,
                    vk::SampleCountFlagBits::e1,
                    vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eDepthStencilAttachment,
                    vk::SharingMode::eExclusive,
                    {},
                    vk::ImageLayout::eUndefined},
    } {
        view = {
                device,
                {
                        {},
                        image,
                        vk::ImageViewType::e2D,
                        DEPTH_FORMAT,
                        {},
                        {vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1},
                },
        };
    }

    DepthImage(DepthImage&&) = default;

    DepthImage& operator=(DepthImage&&) = default;
};
