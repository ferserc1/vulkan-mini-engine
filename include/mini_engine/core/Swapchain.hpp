#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace miniengine {

class VulkanData;

namespace core {

class Swapchain {
public:

    void init(VulkanData * vulkanData, uint32_t width, uint32_t height);
    void cleanup();

protected:
    VkSwapchainKHR _swapchain;
    VkFormat _imageFormat;

    std::vector<VkImage> _images;
    std::vector<VkImageView> _imageViews;
    VkExtent2D _extent;

    VulkanData* _vulkanData = nullptr;
};

}
}
