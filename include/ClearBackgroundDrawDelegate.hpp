#pragma once

#include <vkme/core/Image.hpp>
#include <vkme/VulkanData.hpp>
#include <vkme/DrawLoop.hpp>

class ClearBackgroundDrawDelegate : public vkme::DrawLoopDelegate {
public:
    void init(vkme::VulkanData * vulkanData);
    VkImageLayout draw(VkCommandBuffer cmd, VkImage swapchainImage, VkExtent2D imageExtent, uint32_t currentFrame);

protected:
    std::shared_ptr<vkme::core::Image> _drawImage;
    
    void drawBackground(VkCommandBuffer cmd, uint32_t currentFrame);
};
