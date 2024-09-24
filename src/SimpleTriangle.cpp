
#include <SimpleTriangle.hpp>

#include <vkme/factory/GraphicsPipeline.hpp>

#include <vkme/core/Info.hpp>

void SimpleTriangleDelegate::init(vkme::VulkanData * vd)
{
    _vulkanData = vd;
    
    initPipeline();
}

VkImageLayout SimpleTriangleDelegate::draw(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const vkme::core::Image* colorImage,
    const vkme::core::Image* depthImage,
    vkme::core::FrameResources& frameResources
) {

    vkme::core::Image::cmdTransitionImage(cmd, colorImage->image(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    
    // Draw background
    VkClearColorValue clearValue = {};
    float flash = std::abs(std::sin(currentFrame / 120.0f));
    clearValue = { { flash, 0.0f, 1.0f, 1.0f } };
    auto clearRange = vkme::core::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(
        cmd,
        colorImage->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        &clearValue,
        1,
        &clearRange
    );
    
    
    vkme::core::Image::cmdTransitionImage(cmd, colorImage->image(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    
    auto colorAttachment = vkme::core::Info::attachmentInfo(colorImage->imageView(), nullptr);
    auto renderInfo = vkme::core::Info::renderingInfo(colorImage->extent2D(), &colorAttachment, nullptr);
    vkme::core::cmdBeginRendering(cmd, &renderInfo);
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    
    cmdSetDefaultViewportAndScissor(cmd, colorImage->extent2D());
    
    vkCmdDraw(cmd, 3, 1, 0, 0);

	vkme::core::cmdEndRendering(cmd);
    
    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

void SimpleTriangleDelegate::initPipeline()
{
    vkme::factory::GraphicsPipeline pipelineFactory(_vulkanData);
    
    pipelineFactory.addShader("colored_triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    pipelineFactory.addShader("simple_vertex_color.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    auto layoutInfo = vkme::core::Info::pipelineLayoutInfo();
    VK_ASSERT(vkCreatePipelineLayout(_vulkanData->device(), &layoutInfo, nullptr, &_layout));
    pipelineFactory.setColorAttachmentFormat(_vulkanData->swapchain().imageFormat());
    _pipeline = pipelineFactory.build(_layout);
    
    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipelineLayout(dev, _layout, nullptr);
    });
    
}
