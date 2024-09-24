#include <TexturesTestDelegate.hpp>
#include <vkme/factory/GraphicsPipeline.hpp>
#include <vkme/core/Info.hpp>
#include <vkme/factory/DescriptorSetLayout.hpp>

#include <vkme/PlatformTools.hpp>

void TexturesTestDelegate::init(vkme::VulkanData * vulkanData)
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
    
    vulkanData->cleanupManager().push([this](VkDevice) {
        this->cleanup();
    });
    
    initMesh();
    
    initPipeline();
}

void TexturesTestDelegate::initFrameResources(vkme::core::DescriptorSetAllocator * allocator)
{

    allocator->initPool(1000, {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 }
    });
}

void TexturesTestDelegate::swapchainResized(VkExtent2D newExtent)
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
    
    // Resize the projection matrix
    /// First update the projection matrix to match the
    glm::mat4 proj = glm::perspective(glm::radians(70.0f), float(newExtent.width) / float(newExtent.height), 100.0f, 0.1f);
    proj[1][1] *= -1.0f;
    _sceneData.proj = proj;
    _sceneData.viewProj = proj * _sceneData.view;
}

void TexturesTestDelegate::cleanup()
{
    _drawImage->cleanup();
}

VkImageLayout TexturesTestDelegate::draw(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const vkme::core::Image* colorImage,
    const vkme::core::Image* depthImage,
    vkme::core::FrameResources& frameResources
) {
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
    
    drawGeometry(cmd, _drawImage->imageView(), colorImage->extent2D(), depthImage, currentFrame, frameResources);
    
    // Transition _drawImage and swapchain image to copy the first one to the second one
    core::Image::cmdTransitionImage(
        cmd,
        _drawImage->image(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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

void TexturesTestDelegate::drawUI()
{
    ImGui::ShowDemoWindow();
}

void TexturesTestDelegate::initPipeline()
{
    // TODO: Init scene data. Extract to other function
    auto viewportExtent = _vulkanData->swapchain().extent();
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f));
    // We are using a technique that consist in reversing the depth test (1 is the near plane and 0 is the far plane).
    // This technique increases the quality of the depth test.
    glm::mat4 proj = glm::perspective(glm::radians(70.0f), float(viewportExtent.width) / float(viewportExtent.height), 100.0f, 0.1f);
    proj[1][1] *= -1.0f;
    _sceneData.view = view;
    _sceneData.proj = proj;
    _sceneData.viewProj = proj * view;
    _sceneData.ambientColor = glm::vec4{0.1f, 0.1f, 0.1f, 1.0f};
    _sceneData.sunlightColor = glm::vec4{0.9, 0.87, 0.82, 1.0f};
    _sceneData.sunlightDirection = glm::vec4{5.0, 5.0, 0.0, 1.0};
    
    vkme::factory::GraphicsPipeline plFactory(this->_vulkanData);
    
    plFactory.addShader("textures_test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("textures_test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    // Descriptor set layout to pass the frame data to the shader
    vkme::factory::DescriptorSetLayout dsFactory;
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    _sceneDataDescriptorLayout = dsFactory.build(_vulkanData->device(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkPushConstantRange bufferRange = {};
    bufferRange.offset = 0;
    bufferRange.size = sizeof(vkme::geo::MeshPushConstants);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    auto layoutInfo = vkme::core::Info::pipelineLayoutInfo();
    layoutInfo.pPushConstantRanges = &bufferRange;
    layoutInfo.pushConstantRangeCount = 1;
    VkDescriptorSetLayout setLayouts[] = { _sceneDataDescriptorLayout };
    layoutInfo.pSetLayouts = setLayouts;
    layoutInfo.setLayoutCount = 1;
    
    VK_ASSERT(vkCreatePipelineLayout(_vulkanData->device(), &layoutInfo, nullptr, &_pipelineLayout));
    
    plFactory.setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
    plFactory.setDepthFormat(_vulkanData->swapchain().depthImageFormat());
    plFactory.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(true, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    //plFactory.enableBlendingAdditive();
    //plFactory.disableDepthtest();
    _pipeline = plFactory.build(_pipelineLayout);
    
    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
    });
    
}

void TexturesTestDelegate::initMesh()
{
    std::string assetsPath = vkme::PlatformTools::assetPath() + "basicmesh.glb";
    
    _models = vkme::geo::Model::loadGltf(_vulkanData, assetsPath);
    
    _vulkanData->cleanupManager().push([&](VkDevice) {
        for (auto m : _models) {
            m->cleanup();
        }
    });
}

void TexturesTestDelegate::drawBackground(VkCommandBuffer cmd, uint32_t currentFrame)
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

void TexturesTestDelegate::drawGeometry(
    VkCommandBuffer cmd,
    VkImageView currentImage,
    VkExtent2D imageExtent,
    const vkme::core::Image* depthImage,
    uint32_t currentFrame,
    vkme::core::FrameResources& frameResources
) {
    // This is an example on how to pass temporary data to the shader, allocating
    // descriptor sets and buffers only for one frame. Here we are passing the
    // scene light data and the view and projection matrixes, is not the best
    // example, because this information is not temporary, but its a valid example
    // on how to do it.
    auto sceneDataBuffer = vkme::core::Buffer::createAllocatedBuffer(
        _vulkanData,
        sizeof(SceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    
    // Add the sceneDataBuffer to the cleanup manager of the frame resources.
    // This data will be destroyed after the frame rendering
    frameResources.cleanupManager.push([&, sceneDataBuffer](VkDevice) {
        sceneDataBuffer->cleanup();
        delete sceneDataBuffer;
    });
    
    // Write the buffer data
    SceneData* sceneDataPtr = reinterpret_cast<SceneData*>(sceneDataBuffer->allocatedData());
    *sceneDataPtr = _sceneData;
    
    // Create the descriptor set. All the descriptor sets created with the frame
    // resources descriptor set allocator will be cleaned after the frame render
    auto sceneDS = std::unique_ptr<vkme::core::DescriptorSet>(
        frameResources.descriptorAllocator->allocate(_sceneDataDescriptorLayout)
    );
    sceneDS->updateBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sceneDataBuffer, sizeof(SceneData), 0);
    
    
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
    
    pushConstants.modelMatrix = glm::rotate(glm::mat4(1.0), glm::radians(float(currentFrame % 360)), glm::vec3(0.0f, 1.0f, 0.0f));
        
    pushConstants.vertexBufferAddress = _models[2]->meshBuffers()->vertexBufferAddress;
    
    vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vkme::geo::MeshPushConstants), &pushConstants);
    vkCmdBindIndexBuffer(cmd, _models[2]->meshBuffers()->indexBuffer->buffer(), 0, VK_INDEX_TYPE_UINT32);

    // Bind the scene descriptor set
    VkDescriptorSet ds[] = {
        sceneDS->descriptorSet()
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, ds, 0, nullptr);
    
    vkCmdDrawIndexed(cmd, _models[2]->surface(0).indexCount, 1, _models[2]->surface(0).startIndex, 0, 0);
    
    vkme::core::cmdEndRendering(cmd);
}
