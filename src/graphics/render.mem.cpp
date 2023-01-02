#include "render.hpp"

DepthImage::DepthImage(VulkanContext& vk) : Image(
        vk.allocator,
        VmaAllocationCreateInfo{
                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        },
        vk::ImageCreateInfo{
                {},
                vk::ImageType::e2D,
                vk::Format::eD32Sfloat,
                vk::Extent3D{vk.window.extent(), 1},
                1,
                1,
                vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment,
                vk::SharingMode::eExclusive,
                {},
                vk::ImageLayout::eUndefined
        }) {
    view = {vk.device, {
            {},
            image,
            vk::ImageViewType::e2D,
            DEPTH_FORMAT,
            {},
            {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}
    }};
}

Image::Image(MemAllocator const& allocator, VmaAllocationCreateInfo const& create_info, vk::ImageCreateInfo const& image_info)
        : allocator{allocator} {
    auto create_info_raw = static_cast<VkImageCreateInfo>(image_info);
    VkImage image_raw;
    vmaCreateImage(*allocator, &create_info_raw, &create_info, &image_raw, &allocation, nullptr);
    image = image_raw;
}

Image::~Image() {
    if (allocator && image) {
        vmaDestroyImage(*allocator, static_cast<VkImage>(image), allocation);
    }
}