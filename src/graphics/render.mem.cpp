#include "render.hpp"

Image::Image(MemAllocator const& alloc, VmaAllocationCreateInfo const& create_info, vk::ImageCreateInfo const& image_info) {
    auto create_info_raw = static_cast<VkImageCreateInfo>(image_info);
    VkImage image_raw;
    vmaCreateImage(*alloc, &create_info_raw, &create_info, &image_raw, &allocation, nullptr);
    image = image_raw;
}

Image::~Image() {
    if (!allocator) return;
    GAME_ASSERT(image);
    vmaDestroyImage(*allocator, static_cast<VkImage>(image), allocation);
}