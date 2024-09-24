#pragma once

#include <vkme/core/Image.hpp>
#include <vkme/VulkanData.hpp>
#include <vkme/DrawLoop.hpp>
#include <vkme/UserInterface.hpp>

class ColorTriangleDelegate : public vkme::DrawLoopDelegate, public vkme::UserInterfaceDelegate {
public:
    void init(vkme::VulkanData * vulkanData);

    void cleanup();
    
    void swapchainResized(VkExtent2D newExtent);
    
    VkImageLayout draw(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const vkme::core::Image* colorImage,
        const vkme::core::Image* depthImage,
        vkme::core::FrameResources& frameResources
    );

    void drawUI();

protected:
    vkme::VulkanData * _vulkanData;
    
    std::shared_ptr<vkme::core::Image> _drawImage;
    
    VkPipelineLayout _pipelineLayout;
    VkPipeline _pipeline;
    
    void initPipeline();

    void drawBackground(VkCommandBuffer cmd, uint32_t currentFrame);
    void drawGeometry(VkCommandBuffer cmd, VkImageView currentImage, VkExtent2D imageExtent);
};
