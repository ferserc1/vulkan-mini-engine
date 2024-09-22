#include <TestModelDelegate.hpp>
#include <vkme/factory/GraphicsPipeline.hpp>
#include <vkme/core/Info.hpp>

#include <vkme/PlatformTools.hpp>

void TestModelDelegate::init(vkme::VulkanData * vulkanData)
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
    
    initMesh();
    
    initPipeline();
}

void TestModelDelegate::swapchainResized(VkExtent2D newExtent)
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
}

void TestModelDelegate::cleanup()
{
    _drawImage->cleanup();
}

VkImageLayout TestModelDelegate::draw(VkCommandBuffer cmd, VkImage swapchainImage, VkExtent2D imageExtent, uint32_t currentFrame, const vkme::core::Image* depthImage)
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
    
    drawGeometry(cmd, _drawImage->imageView(), imageExtent, depthImage, currentFrame);
    
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

void TestModelDelegate::drawUI()
{
    ImGui::ShowDemoWindow();
}

void TestModelDelegate::initPipeline()
{
    vkme::factory::GraphicsPipeline plFactory(this->_vulkanData);
    
    plFactory.addShader("colored_mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("simple_vertex_color.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkPushConstantRange bufferRange = {};
    bufferRange.offset = 0;
    bufferRange.size = sizeof(vkme::geo::MeshPushConstants);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    auto layoutInfo = vkme::core::Info::pipelineLayoutInfo();
    layoutInfo.pPushConstantRanges = &bufferRange;
    layoutInfo.pushConstantRangeCount = 1;
    
    VK_ASSERT(vkCreatePipelineLayout(_vulkanData->device(), &layoutInfo, nullptr, &_pipelineLayout));
    plFactory.setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
    plFactory.setDepthFormat(_vulkanData->swapchain().depthImageFormat());
    plFactory.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(true, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    //plFactory.enableBlendingAdditive();
    //plFactory.disableDepthtest();
    _pipeline = plFactory.build(_pipelineLayout);
    
    _vulkanData->cleanupManager().push([&]() {
        vkDestroyPipeline(_vulkanData->device(), _pipeline, nullptr);
        vkDestroyPipelineLayout(_vulkanData->device(), _pipelineLayout, nullptr);
    });
    
}

void TestModelDelegate::initMesh()
{
    std::string assetsPath = vkme::PlatformTools::assetPath() + "basicmesh.glb";
    
    _models = vkme::geo::Model::loadGltf(_vulkanData, assetsPath);
    
    _vulkanData->cleanupManager().push([&]() {
        for (auto m : _models) {
            m->cleanup();
        }
    });
}

void TestModelDelegate::drawBackground(VkCommandBuffer cmd, uint32_t currentFrame)
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

void TestModelDelegate::drawGeometry(
    VkCommandBuffer cmd,
    VkImageView currentImage,
    VkExtent2D imageExtent,
    const vkme::core::Image* depthImage,
    uint32_t currentFrame
) {
    auto colorAttachment = vkme::core::Info::attachmentInfo(currentImage, nullptr);
    auto depthAttachment = vkme::core::Info::depthAttachmentInfo(depthImage->imageView(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    auto renderInfo = vkme::core::Info::renderingInfo(imageExtent, &colorAttachment, &depthAttachment);
    
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
    
    vkme::geo::MeshPushConstants pushConstants;
    
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f));
    // We are using a technique that consist in reversing the depth test (1 is the near plane and 0 is the far plane).
    // This technique increases the quality of the depth test.
    glm::mat4 proj = glm::perspective(glm::radians(70.0f), float(imageExtent.width) / float(imageExtent.height), 100.0f, 0.1f);
    proj[1][1] *= -1.0f;
    
    pushConstants.modelMatrix = proj * view *
        glm::rotate(glm::mat4(1.0), glm::radians(float(currentFrame % 360)), glm::vec3(0.0f, 1.0f, 0.0f));
        
    
    pushConstants.vertexBufferAddress = _models[2]->meshBuffers()->vertexBufferAddress;
    
    vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vkme::geo::MeshPushConstants), &pushConstants);
    vkCmdBindIndexBuffer(cmd, _models[2]->meshBuffers()->indexBuffer->buffer(), 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdDrawIndexed(cmd, _models[2]->surface(0).indexCount, 1, _models[2]->surface(0).startIndex, 0, 0);
    
    vkme::core::cmdEndRendering(cmd);
}
