#include <ColorTriangleDelegate.hpp>
#include <vkme/factory/GraphicsPipeline.hpp>
#include <vkme/core/Info.hpp>

#include <vkme/PlatformTools.hpp>

void ColorTriangleDelegate::init(vkme::VulkanData * vulkanData)
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
        _drawImage->cleanup();
    });
    
    initPipeline();
}

VkImageLayout ColorTriangleDelegate::draw(VkCommandBuffer cmd, VkImage swapchainImage, VkExtent2D imageExtent, uint32_t currentFrame)
{
    using namespace vkme;
    
    // Transition draw image to render on it
    core::Image::cmdTransitionImage(
        cmd,
        _drawImage->image(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL
    );
    
    drawBackground(cmd, currentFrame);
    
    core::Image::cmdTransitionImage(
        cmd,
        _drawImage->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    
    drawGeometry(cmd, _drawImage->imageView(), imageExtent);
    
    // Transition _drawImage and swapchain image to copy the first one to the second one
    core::Image::cmdTransitionImage(
        cmd,
        _drawImage->image(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );
    core::Image::cmdTransitionImage(
        cmd,
        swapchainImage,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    
    // Copy drawImage into swapchain image
    core::Image::cmdCopy(
        cmd,
        _drawImage->image(), _drawImage->extent2D(),
        swapchainImage, imageExtent
    );
    
    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
}

void ColorTriangleDelegate::drawUI()
{
    ImGui::ShowDemoWindow();
}

void ColorTriangleDelegate::initPipeline()
{
    vkme::factory::GraphicsPipeline plFactory(this->_vulkanData);
    
    plFactory.addShader("colored_triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("simple_vertex_color.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    auto layoutInfo = vkme::core::Info::pipelineLayoutInfo();
    VK_ASSERT(vkCreatePipelineLayout(_vulkanData->device(), &layoutInfo, nullptr, &_pipelineLayout));
    plFactory.setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
    _pipeline = plFactory.build(_pipelineLayout);
    
    _vulkanData->cleanupManager().push([&]() {
        vkDestroyPipeline(_vulkanData->device(), _pipeline, nullptr);
        vkDestroyPipelineLayout(_vulkanData->device(), _pipelineLayout, nullptr);
    });
    
}

void ColorTriangleDelegate::drawBackground(VkCommandBuffer cmd, uint32_t currentFrame)
{
    // Clear image
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(currentFrame / 120.0f));
    clearValue = { { 0.0f, 0.0f, flash, 1.0f } };
    auto clearRange = vkme::core::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(
        cmd,
        _drawImage->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        &clearValue, 1, &clearRange
    );
}

void ColorTriangleDelegate::drawGeometry(VkCommandBuffer cmd, VkImageView currentImage, VkExtent2D imageExtent)
{
    auto colorAttachment = vkme::core::Info::attachmentInfo(currentImage, nullptr);
    auto renderInfo = vkme::core::Info::renderingInfo(imageExtent, &colorAttachment, nullptr);
    
    vkme::core::cmdBeginRendering(cmd, &renderInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    
    VkViewport viewport = {};
    viewport.x = 0; viewport.y = 0;
    viewport.width = imageExtent.width; viewport.height = imageExtent.height;
    viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    
    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = imageExtent.width;
    scissor.extent.height = imageExtent.height;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    
    vkCmdDraw(cmd, 3, 1, 0, 0);
    
    vkme::core::cmdEndRendering(cmd);
}
