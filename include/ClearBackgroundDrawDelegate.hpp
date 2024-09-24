#pragma once

#include <vkme/core/Image.hpp>
#include <vkme/VulkanData.hpp>
#include <vkme/DrawLoop.hpp>

class ClearBackgroundDrawDelegate : public vkme::DrawLoopDelegate, public vkme::UserInterfaceDelegate {
public:
    void init(vkme::VulkanData * vulkanData);

    // In this delegate we draw on an image we create, instead of using the swapchain. For that reason we have to
    // implement this function to resize the image we'll use for drawing. If we draw directly to the
    // swapchain, this function may have an empty implementation, but it's mandatory to implement it to make sure that the
    // size of the image is taken into account.
    // that the size of the swapchain is taken into account when implementing the delegate.
    void swapchainResized(VkExtent2D newExtent);

    VkImageLayout draw(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const vkme::core::Image* colorImage,
        const vkme::core::Image* depthImage,
        vkme::core::FrameResources& frameResources
    );

    // We don't draw UI here, but since in the main.cpp file we are setting the class as a delegate for drawing and
    // UI, we must implement this interface
    void drawUI();

    void cleanup();

protected:
    std::shared_ptr<vkme::core::Image> _drawImage;

    vkme::VulkanData * _vulkanData;
    
    void drawBackground(VkCommandBuffer cmd, uint32_t currentFrame);
};
