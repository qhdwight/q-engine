export module render:memory;

import std;
import vma;
import vulkan;
import vulkanc;

constexpr vk::Format DEPTH_FORMAT = vk::Format::eD32SfloatS8Uint;

struct MemAllocator : std::shared_ptr<vma::VmaAllocator> {

    MemAllocator() = default;

    MemAllocator(vk::raii::Instance const& instance, vk::raii::Device const& device, vk::raii::PhysicalDevice const& physicalDevice) {
        vma::VmaVulkanFunctions functions{
                .vkGetInstanceProcAddr = &vkc::vkGetInstanceProcAddr,
                .vkGetDeviceProcAddr = &vkc::vkGetDeviceProcAddr,
        };

        vma::VmaAllocatorCreateInfo create_info{
                .physicalDevice = *physicalDevice,
                .device = *device,
                .pVulkanFunctions = &functions,
                .instance = *instance,
                .vulkanApiVersion = vk::makeApiVersion(0, 1, 3, 0),
        };

        auto* raw = new vma::VmaAllocator{};
        if (vma::vmaCreateAllocator(&create_info, raw) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create memory allocator");
        }
        reset(raw, [](vma::VmaAllocator* p) {
            vma::vmaDestroyAllocator(*p);
            delete p;
        });
    }
};

struct Image {

    MemAllocator allocator;
    vk::Image image;
    vk::raii::ImageView view = nullptr;
    vma::VmaAllocation allocation{};

    Image(const Image&) = delete;

    Image& operator=(Image&) = delete;

    Image(Image&&) = default;

    Image& operator=(Image&&) = default;

    Image(MemAllocator const& allocator, vma::VmaAllocationCreateInfo const& allocInfo, vk::ImageCreateInfo const& imageInfo)
        : allocator{allocator} {

        auto createInfoRaw = static_cast<vkc::VkImageCreateInfo>(imageInfo);
        vkc::VkImage imageRaw;
        vma::vmaCreateImage(*allocator, &createInfoRaw, &allocInfo, &imageRaw, &allocation, nullptr);
        image = imageRaw;
    }

    ~Image() {
        if (!allocator || !image) return;

        vma::vmaDestroyImage(*allocator, image, allocation);
    }
};

struct DepthImage : Image {

    explicit DepthImage(vk::raii::Device const& device, MemAllocator const& allocator, vk::Extent2D const& extent)
        : Image{
                  allocator,
                  vma::VmaAllocationCreateInfo{
                          .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                          .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  },
                  vk::ImageCreateInfo{
                          {},
                          vk::ImageType::e2D,
                          DEPTH_FORMAT,
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
