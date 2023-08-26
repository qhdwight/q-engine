module;

#include <vk_mem_alloc.h>

export module vma;

export {

    using ::VmaAllocation;
    using ::VmaAllocationCreateInfo;
    using ::VmaAllocator;
    using ::VmaAllocatorCreateInfo;
    using ::vmaCreateAllocator;
    using ::vmaCreateImage;
    using ::vmaDestroyAllocator;
    using ::vmaDestroyImage;
    using ::VmaVulkanFunctions;

}// namespace vma
