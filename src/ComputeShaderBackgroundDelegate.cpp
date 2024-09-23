#include <ComputeShaderBackgroundDelegate.hpp>

#include <vkme/factory/DescriptorSetLayout.hpp>
#include <vkme/factory/ComputePipeline.hpp>

void ComputeShaderBackgroundDelegate::init(vkme::VulkanData * vulkanData)
{
    _vulkanData = vulkanData;
    
    _drawImage = std::shared_ptr<vkme::core::Image>(vkme::core::Image::createAllocatedImage(
        vulkanData,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        vulkanData->swapchain().extent(),
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    ));
    
    vulkanData->cleanupManager().push([this] {
        this->cleanup();
    });
    
    initDescriptors();
    
    initPipelines();
}

void ComputeShaderBackgroundDelegate::init(vkme::VulkanData * vulkanData, vkme::UserInterface * ui)
{
    _vulkanData = vulkanData;
}

void ComputeShaderBackgroundDelegate::cleanup()
{
    _drawImage->cleanup();
}

void ComputeShaderBackgroundDelegate::swapchainResized(VkExtent2D newExtent)
{
    // Resize the target imagge
    _drawImage->cleanup();
    _drawImage = std::shared_ptr<vkme::core::Image>(vkme::core::Image::createAllocatedImage(
        _vulkanData,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        newExtent,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    ));

    // Set the new image to the compute shader descriptor set
    _drawImageDescriptors->updateImage(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, _drawImage->imageView(), VK_IMAGE_LAYOUT_GENERAL);
}

VkImageLayout ComputeShaderBackgroundDelegate::draw(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const vkme::core::Image* colorImage,
    const vkme::core::Image* depthImage
) {
    using namespace vkme;
    
    // Transition draw image to render on it
    core::Image::cmdTransitionImage(
        cmd,
        _drawImage->image(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL
    );
    
    drawBackground(cmd, currentFrame, colorImage->extent2D());
    
    // Transition _drawImage and swapchain image to copy the first one to the second one
    core::Image::cmdTransitionImage(
        cmd,
        _drawImage->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );
    core::Image::cmdTransitionImage(
        cmd,
        colorImage->image(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    
    // Copy drawImage into swapchain image
    core::Image::cmdCopy(
        cmd,
        _drawImage->image(), _drawImage->extent2D(),
        colorImage->image(), colorImage->extent2D()
    );
    
    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
}

void ComputeShaderBackgroundDelegate::drawUI()
{
    ImGui::ShowDemoWindow();
}

void ComputeShaderBackgroundDelegate::drawBackground(VkCommandBuffer cmd, uint32_t currentFrame, VkExtent2D imageExtent)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipeline);
    
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        _gradientPipelineLayout,
        0,
        1,
        &(*_drawImageDescriptors),
        0,
        nullptr
    );
    
    vkCmdDispatch(cmd, std::ceil(imageExtent.width / 16.0), std::ceil(imageExtent.height / 16.0), 1);
}



void ComputeShaderBackgroundDelegate::initDescriptors()
{
    using namespace vkme::core;
    std::vector<DescriptorSetAllocator::PoolSizeRatio> sizes = {};

    // One image to pass the draw image to the compute shader
    sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 });
    
    _descriptorAllocator.init(_vulkanData);
    _descriptorAllocator.initPool(_vulkanData->device(), 10, sizes);
    
    vkme::factory::DescriptorSetLayout dsFactory;
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    _drawImageDescriptorLayout = dsFactory.build(_vulkanData->device(), VK_SHADER_STAGE_COMPUTE_BIT);
    
    // Wrapped method: using a vkme::core::DescriptorSet wrapper
    _drawImageDescriptors = std::unique_ptr<DescriptorSet>(_descriptorAllocator.allocate(_drawImageDescriptorLayout));
    _drawImageDescriptors->updateImage(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, _drawImage->imageView(), VK_IMAGE_LAYOUT_GENERAL);
    
    _vulkanData->cleanupManager().push([&] {
        _descriptorAllocator.destroy();
        
        vkDestroyDescriptorSetLayout(_vulkanData->device(), _drawImageDescriptorLayout, nullptr);
    });
}

void ComputeShaderBackgroundDelegate::initPipelines()
{
    using namespace vkme;
    
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pSetLayouts = &_drawImageDescriptorLayout;
    layoutInfo.setLayoutCount = 1;
    
    VK_ASSERT(vkCreatePipelineLayout(
        _vulkanData->device(),
        &layoutInfo,
        nullptr,
        &_gradientPipelineLayout
    ));
    

    factory::ComputePipeline pipelineFactory(_vulkanData);
    pipelineFactory.setShader("gradient.comp.spv");
    _gradientPipeline = pipelineFactory.build(_gradientPipelineLayout);
    
    _vulkanData->cleanupManager().push([&]() {
        vkDestroyPipeline(_vulkanData->device(), _gradientPipeline, nullptr);
        vkDestroyPipelineLayout(_vulkanData->device(), _gradientPipelineLayout, nullptr);
    });
    
}
