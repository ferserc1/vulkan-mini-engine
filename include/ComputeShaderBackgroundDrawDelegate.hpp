#pragma once

#include <vkme/core/Image.hpp>
#include <vkme/VulkanData.hpp>
#include <vkme/DrawLoop.hpp>
#include <vkme/core/DescriptorSetAllocator.hpp>
#include <vkme/core/DescriptorSet.hpp>
#include <memory>


class ComputeShaderBackgroundDrawDelegate : public vkme::DrawLoopDelegate {
public:
    void init(vkme::VulkanData * vulkanData);
    VkImageLayout draw(VkCommandBuffer cmd, VkImage swapchainImage, VkExtent2D imageExtent, uint32_t currentFrame);

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
