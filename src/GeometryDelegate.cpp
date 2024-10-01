#include <GeometryDelegate.hpp>
#include <vkme/factory/GraphicsPipeline.hpp>
#include <vkme/core/Info.hpp>
#include <vkme/factory/DescriptorSetLayout.hpp>
#include <vkme/geo/Sphere.hpp>
#include <vkme/geo/Modifiers.hpp>

#include <vkme/PlatformTools.hpp>

void GeometryDelegate::init(vkme::VulkanData * vulkanData)
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
    
    initPipeline();
    
    initScene();
    
    initMesh();
    
}

void GeometryDelegate::initFrameResources(vkme::core::DescriptorSetAllocator * allocator)
{
    allocator->initPool(
        1000,
        {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        }
    );
}

void GeometryDelegate::swapchainResized(VkExtent2D newExtent)
{
    _drawImage->cleanup();
    _drawImage = std::shared_ptr<vkme::core::Image>(vkme::core::Image::createAllocatedImage(
        _vulkanData,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        newExtent,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    ));
    
    glm::mat4 proj = glm::perspective(glm::radians(50.0f), float(newExtent.width) / float(newExtent.height), 0.1f, 10.0f);
    proj[1][1] *= -1.0f;
    proj[0][0] *= -1.0f;
    _sceneData.proj = proj;
    _sceneData.viewProj = proj * _sceneData.view;
}

void GeometryDelegate::cleanup()
{
    _drawImage->cleanup();
}

VkImageLayout GeometryDelegate::draw(
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
    
    core::Image::cmdTransitionImage(
        cmd,
        depthImage->image(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
    
    drawBackground(cmd, currentFrame, depthImage);
    
    core::Image::cmdTransitionImage(
        cmd,
        depthImage->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    );
    
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

void GeometryDelegate::drawUI()
{
    if (ImGui::Begin("Rotation"))
    {
        if (_rotateAxis == 0)
        {
            ImGui::Text("No rotation");
        }
        else if (_rotateAxis == 1)
        {
            ImGui::Text("Rotate X");
        }
        else if (_rotateAxis == 2)
        {
            ImGui::Text("Rotate Y");
        }
        else if (_rotateAxis == 3)
        {
            ImGui::Text("Rotate Z");
        }
        
        ImGui::SliderInt("Rotation axis", reinterpret_cast<int*>(&_rotateAxis), 0, 3);
        
        ImGui::Checkbox("Transparent", &_transparentMaterial);
    }
    ImGui::End();
}

void GeometryDelegate::initPipeline()
{
    vkme::factory::GraphicsPipeline plFactory(this->_vulkanData);
    
    // Load shaders
    plFactory.addShader("textures_test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("textures_test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    
    vkme::factory::DescriptorSetLayout dsFactory;
    
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    _sceneDataDescriptorLayout = dsFactory.build(_vulkanData->device(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    
    dsFactory.clear();
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _imageDescriptorLayout = dsFactory.build(_vulkanData->device(), VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkPushConstantRange bufferRange = {};
    bufferRange.offset = 0;
    bufferRange.size = sizeof(vkme::geo::MeshPushConstants);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    auto layoutInfo = vkme::core::Info::pipelineLayoutInfo();
    layoutInfo.pPushConstantRanges = &bufferRange;
    layoutInfo.pushConstantRangeCount = 1;
    VkDescriptorSetLayout setLayouts[] = {
        _sceneDataDescriptorLayout,   // Scene data set layout
        _imageDescriptorLayout        // Per-surface descriptor set layout
    };
    layoutInfo.pSetLayouts = setLayouts;
    layoutInfo.setLayoutCount = 2;
    
    VK_ASSERT(vkCreatePipelineLayout(_vulkanData->device(), &layoutInfo, nullptr, &_pipelineLayout));
    
    plFactory.setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
    plFactory.setDepthFormat(_vulkanData->swapchain().depthImageFormat());
    plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(true, VK_FRONT_FACE_CLOCKWISE);
    
    _pipeline = plFactory.build(_pipelineLayout);
    
    plFactory.enableBlendingAdditive();
    plFactory.disableDepthtest();
    
    _transparentPipeline = plFactory.build(_pipelineLayout);
    
    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipeline(dev, _transparentPipeline, nullptr);
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _sceneDataDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _imageDescriptorLayout, nullptr);
    });
    
}

void GeometryDelegate::initScene()
{
    auto viewportExtent = _vulkanData->swapchain().extent();
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f));
    
    //glm::mat4 proj = glm::perspective(glm::radians(50.0f), float(viewportExtent.width) / float(viewportExtent.height), 100.0f, 0.1f);
    glm::mat4 proj = glm::perspective(glm::radians(50.0f), float(viewportExtent.width) / float(viewportExtent.height), 0.1f, 10.0f);
    proj[1][1] *= -1.0f;
    proj[0][0] *= -1.0f;
    
    _sceneData.view = view;
    _sceneData.proj = proj;
    _sceneData.viewProj = proj * view;
    _sceneData.ambientColor = glm::vec4{0.1f, 0.1f, 0.1f, 1.0f};
    _sceneData.sunlightColor = glm::vec4{0.9, 0.87, 0.82, 1.0f};
    _sceneData.sunlightDirection = glm::vec4{5.0, 5.0, 0.0, 1.0};
}

void GeometryDelegate::initMesh()
{
    const std::string imagePath = vkme::PlatformTools::assetPath() + "country_field_sun.jpg";
    _textureImage = std::unique_ptr<vkme::core::Image>(
        vkme::core::Image::loadImage(_vulkanData, imagePath)
    );
    
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(_vulkanData->device(), &samplerInfo, nullptr, &_imageSampler);
    
    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        _textureImage->cleanup();
        vkDestroySampler(dev, _imageSampler, nullptr);
    });
    
    _materialDescriptorSetAllocator = std::unique_ptr<vkme::core::DescriptorSetAllocator>(
        new vkme::core::DescriptorSetAllocator()
    );
    _materialDescriptorSetAllocator->init(_vulkanData);
    
    _materialDescriptorSetAllocator->initPool(1000, {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    });
    _vulkanData->cleanupManager().push([&](VkDevice) {
        _materialDescriptorSetAllocator->clearDescriptors();
        _materialDescriptorSetAllocator->destroy();
    });
    
    _models = {
        vkme::geo::Sphere::createUvSphere(_vulkanData, 1.5f)
    };
    
    for (auto m : _models)
    {
        m->allocateMaterialDescriptorSets(_materialDescriptorSetAllocator.get(), _imageDescriptorLayout);
        
        m->updateDescriptorSets([&](vkme::core::DescriptorSet* ds) {
            ds->updateImage(
                0,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                _textureImage->imageView(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _imageSampler
            );
        });
    }
    
    _vulkanData->cleanupManager().push([&](VkDevice) {
        for (auto m : _models) {
            m->cleanup();
        }
    });
}

void GeometryDelegate::drawBackground(VkCommandBuffer cmd, uint32_t currentFrame, const vkme::core::Image* depthImage)
{
    VkClearColorValue clearValue;
    float r = std::abs(std::sin(currentFrame / 90.0f)) * 0.5f;
    float g = std::abs(std::sin(currentFrame / 180.0f)) * 0.5f;
    float b = std::abs(std::sin(currentFrame / 120.0f)) * 0.5f;
    clearValue = { { r, g, b, 1.0f } };
    auto clearRange = vkme::core::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(
        cmd,
        _drawImage->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        &clearValue, 1, &clearRange
    );
}

void GeometryDelegate::drawGeometry(
    VkCommandBuffer cmd,
    VkImageView currentImage,
    VkExtent2D imageExtent,
    const vkme::core::Image* depthImage,
    uint32_t currentFrame,
    vkme::core::FrameResources& frameResources
) {
    auto sceneDataBuffer = vkme::core::Buffer::createAllocatedBuffer(
        _vulkanData,
        sizeof(SceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    
    frameResources.cleanupManager.push([&, sceneDataBuffer](VkDevice) {
        sceneDataBuffer->cleanup();
        delete sceneDataBuffer;
    });
    
    SceneData* sceneDataPtr = reinterpret_cast<SceneData*>(sceneDataBuffer->allocatedData());
    *sceneDataPtr = _sceneData;
    
    auto sceneDS = std::unique_ptr<vkme::core::DescriptorSet>(
        frameResources.descriptorAllocator->allocate(_sceneDataDescriptorLayout)
    );
    sceneDS->updateBuffer(
        0, // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        sceneDataBuffer,
        sizeof(SceneData),
        0
    );
    
    for (auto& m : _models) {
        glm::mat4 modelMatrix{ 1.0 };
        if (_rotateAxis == 1)
        {
            modelMatrix = glm::rotate(glm::mat4(1.0), glm::radians(float(currentFrame % 360)), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        else if (_rotateAxis == 2)
        {
            modelMatrix = glm::rotate(glm::mat4(1.0), glm::radians(float(currentFrame % 360)), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else if (_rotateAxis == 3)
        {
            modelMatrix = glm::rotate(glm::mat4(1.0), glm::radians(float(currentFrame % 360)), glm::vec3(0.0f, 0.0f, 1.0f));
        }
        m->setModelMatrix(modelMatrix);
    }

    auto colorAttachment = vkme::core::Info::attachmentInfo(currentImage, nullptr);
    auto depthAttachment = vkme::core::Info::depthAttachmentInfo(depthImage->imageView(), 1.0);
    auto renderInfo = vkme::core::Info::renderingInfo(imageExtent, &colorAttachment, &depthAttachment);
    
    vkme::core::cmdBeginRendering(cmd, &renderInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _transparentMaterial ? _transparentPipeline : _pipeline );
    
    cmdSetDefaultViewportAndScissor(cmd, imageExtent);
    
    for (auto m : _models)
    {
        vkme::core::DescriptorSet* ds[] = { sceneDS.get() };
        m->draw(cmd, _pipelineLayout, ds, 1);
    }
    
    
    vkme::core::cmdEndRendering(cmd);
}
