#pragma once

#include <vkme/core/Image.hpp>
#include <vkme/VulkanData.hpp>
#include <vkme/DrawLoop.hpp>
#include <vkme/core/DescriptorSetAllocator.hpp>
#include <vkme/core/DescriptorSet.hpp>
#include <vkme/UserInterface.hpp>
#include <memory>


/*
    This example draws a background over the render image using a compute shader such as
    DrawLoopDelegate. In addition, it also implements UserInterfaceDelegate to draw the
    IMGUI demo user interface.
 */
class ComputeShaderBackgroundDelegate : public vkme::DrawLoopDelegate, public vkme::UserInterfaceDelegate {
public:
    void init(vkme::VulkanData * vulkanData);

    void init(vkme::VulkanData * vulkanData, vkme::UserInterface * ui);

    void cleanup();
    
    void swapchainResized(VkExtent2D newExtent);

    VkImageLayout draw(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const vkme::core::Image* colorImage,
        const vkme::core::Image* depthImage
    );
    
    void drawUI();

protected:
    
    // Store the VulkanData object, to get access to the device, swapchain, allocator and other resources
    vkme::VulkanData * _vulkanData;

    // Use this image to render the background, instead of the swapchain
    std::shared_ptr<vkme::core::Image> _drawImage;
    
    // Descriptor allocator and descriptor set to setup the compute shader layout
    vkme::core::DescriptorSetAllocator _descriptorAllocator;
    
    // Descriptor set wrapper
    std::unique_ptr<vkme::core::DescriptorSet> _drawImageDescriptors;
    
    // Pipeline to process the compute shader
    VkPipeline _gradientPipeline;
    VkPipelineLayout _gradientPipelineLayout;
    
    VkDescriptorSetLayout _drawImageDescriptorLayout;
    
    void drawBackground(VkCommandBuffer cmd, uint32_t currentFrame, VkExtent2D imageExtent);
    
    void initDescriptors();
    
    void initPipelines();
};
