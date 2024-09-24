#pragma once

#include <vkme/DrawLoop.hpp>
#include <vkme/UserInterface.hpp>

class SimpleTriangleDelegate :
    // Required to draw
    public vkme::DrawLoopDelegate,
    
    // Required to use ImGui user interfaces. In this case
    // we aren't using UI, but in the main.cpp file we are
    // using the delegate as DrawLoopDelegate and
    // UserInterfaceDelegate
    public vkme::UserInterfaceDelegate
{
public:
    void init(vkme::VulkanData * vulkanData);
    
    // We don't need to do anything on swapchain resize, because we are
    // drawing directly on the swap chain image
    void swapchainResized(VkExtent2D) {}
    
    VkImageLayout draw(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const vkme::core::Image* colorImage,
        const vkme::core::Image* depthImage,
        vkme::core::FrameResources& frameResources
    );
    
    // We don't need UI
    void drawUI() {}

protected:
    vkme::VulkanData * _vulkanData;
    
    VkPipelineLayout _layout;
    VkPipeline _pipeline;
    
    void initPipeline();
};
