#pragma once

#include <mini_engine/core/Image.hpp>
#include <mini_engine/VulkanData.hpp>
#include <mini_engine/DrawLoop.hpp>

class ClearBackgroundDrawDelegate : public miniengine::DrawLoopDelegate {
public:
    void init(miniengine::VulkanData * vulkanData);
    void draw(VkCommandBuffer cmd, VkImage swapchainImage, VkExtent2D imageExtent, uint32_t currentFrame);

protected:
    std::shared_ptr<miniengine::core::Image> _drawImage;
    
    void drawBackground(VkCommandBuffer cmd, uint32_t currentFrame);
};
