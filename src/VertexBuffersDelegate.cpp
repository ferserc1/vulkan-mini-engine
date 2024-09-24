
#include <VertexBuffersDelegate.hpp>

#include <vkme/factory/GraphicsPipeline.hpp>

#include <vkme/core/Info.hpp>

void VertexBuffersDelegate::init(vkme::VulkanData * vd)
{
    _vulkanData = vd;
    
    initPipeline();
    
    initMesh();
}

VkImageLayout VertexBuffersDelegate::draw(
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
    
    VkBuffer vertexBuffers[] = {_vertexBuffer->buffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdDraw(cmd, uint32_t(_vertices.size()), 1, 0, 0);

    vkme::core::cmdEndRendering(cmd);
    
    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

void VertexBuffersDelegate::initPipeline()
{
    vkme::factory::GraphicsPipeline pipelineFactory(_vulkanData);
    
    pipelineFactory.addShader("vertex_buffer_mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    pipelineFactory.addShader("simple_vertex_color.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    
    pipelineFactory.vertexInputState.vertexBindingDescriptionCount = 1;
    pipelineFactory.vertexInputState.pVertexBindingDescriptions = &bindingDescription;
    pipelineFactory.vertexInputState.vertexAttributeDescriptionCount = uint32_t(attributeDescriptions.size());
    pipelineFactory.vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    auto layoutInfo = vkme::core::Info::pipelineLayoutInfo();
    VK_ASSERT(vkCreatePipelineLayout(_vulkanData->device(), &layoutInfo, nullptr, &_layout));
    pipelineFactory.setColorAttachmentFormat(_vulkanData->swapchain().imageFormat());
    _pipeline = pipelineFactory.build(_layout);
    
    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipelineLayout(dev, _layout, nullptr);
    });
    
}

void VertexBuffersDelegate::initMesh()
{
    auto bufferSize = sizeof(_vertices[0]) * _vertices.size();
    auto stagingBuffer = std::unique_ptr<vkme::core::Buffer>(vkme::core::Buffer::createAllocatedBuffer(
        _vulkanData,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    ));
    
    void* dataPtr = stagingBuffer->allocatedData();
    
    memcpy(dataPtr, _vertices.data(), bufferSize);
    
    _vertexBuffer = vkme::core::Buffer::createAllocatedBuffer(
        _vulkanData,
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );
    
    _vulkanData->command().immediateSubmit([&](VkCommandBuffer cmd) {
        VkBufferCopy vertexCopy = {};
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = bufferSize;
        vkCmdCopyBuffer(cmd, stagingBuffer->buffer(), _vertexBuffer->buffer(), 1, &vertexCopy);
    });
    
    stagingBuffer->cleanup();
    
    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        _vertexBuffer->cleanup();
    });
}
