#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace vkme {

class VulkanData;

namespace core {

class Swapchain {
public:

    void init(VulkanData * vulkanData, uint32_t width, uint32_t height);
    void cleanup();
    
    inline VkSwapchainKHR swapchain() const { return _swapchain; }
    inline VkFormat imageFormat() const { return _imageFormat; }
    inline const std::vector<VkImage>& images() const { return _images; }
    inline const std::vector<VkImageView>& imageViews() const { return _imageViews; }
    inline const VkExtent2D& extent() const { return _extent; }
    inline VkImage image(uint32_t index) const { return _images[index]; }
    inline VkImageView imageView(uint32_t index) const { return _imageViews[index]; }

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
