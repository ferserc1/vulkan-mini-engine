#include <SkySphereDelegate.hpp>
#include <vkme/factory/GraphicsPipeline.hpp>
#include <vkme/core/Info.hpp>
#include <vkme/factory/DescriptorSetLayout.hpp>
#include <vkme/geo/Sphere.hpp>
#include <vkme/geo/Modifiers.hpp>

#include <vkme/PlatformTools.hpp>

void SkySphereDelegate::init(vkme::VulkanData * vulkanData)
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

void SkySphereDelegate::initFrameResources(vkme::core::DescriptorSetAllocator * allocator)
{
}

void SkySphereDelegate::swapchainResized(VkExtent2D newExtent)
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
    
    _sceneData.viewProj = proj * _sceneData.view;
    
    _sceneDataBuffer->cleanup();
    
    _sceneDataBuffer = std::unique_ptr<vkme::core::Buffer>(vkme::core::Buffer::createAllocatedBuffer(
        _vulkanData,
        sizeof(SceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    ));

    SceneData* sceneDataPtr = reinterpret_cast<SceneData*>(_sceneDataBuffer->allocatedData());
    *sceneDataPtr = _sceneData;
    
    _sceneDataDescriptorSet->updateBuffer(
        0, // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        _sceneDataBuffer.get(),
        sizeof(SceneData),
        0
    );
}

void SkySphereDelegate::cleanup()
{
    _sceneDataBuffer->cleanup();
    _drawImage->cleanup();
}

VkImageLayout SkySphereDelegate::draw(
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
        VK_IMAGE_LAYOUT_GENERAL
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

void SkySphereDelegate::drawUI()
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

void SkySphereDelegate::initPipeline()
{
    // Init the descriptor set allocator
    _descriptorSetAllocator = std::unique_ptr<vkme::core::DescriptorSetAllocator>(
        new vkme::core::DescriptorSetAllocator()
    );
    _descriptorSetAllocator->init(_vulkanData);
    
    _descriptorSetAllocator->initPool(1000, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    });
    _vulkanData->cleanupManager().push([&](VkDevice) {
        _descriptorSetAllocator->clearDescriptors();
        _descriptorSetAllocator->destroy();
    });
    
    vkme::factory::GraphicsPipeline plFactory(this->_vulkanData);
    
    // Load shaders
    plFactory.addShader("skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    
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
        _imageDescriptorLayout        // Per-surface descriptor set layout always in the las position
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
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
    });
    
}

void SkySphereDelegate::initScene()
{
    // Allocate scene data descriptor set
    _sceneDataDescriptorSet = std::unique_ptr<vkme::core::DescriptorSet>(
        _descriptorSetAllocator->allocate(_sceneDataDescriptorLayout)
    );
    
    // Update scene data buffer
    auto viewportExtent = _vulkanData->swapchain().extent();
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f));
    glm::mat4 proj = glm::perspective(glm::radians(50.0f), float(viewportExtent.width) / float(viewportExtent.height), 0.1f, 10.0f);
    proj[1][1] *= -1.0f;
    proj[0][0] *= -1.0f;
    
    _sceneData.view = view;
    _sceneData.proj = proj;
    _sceneData.viewProj = proj * view;
    
    _sceneDataBuffer = std::unique_ptr<vkme::core::Buffer>(vkme::core::Buffer::createAllocatedBuffer(
        _vulkanData,
        sizeof(SceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    ));
    
    SceneData* sceneDataPtr = reinterpret_cast<SceneData*>(_sceneDataBuffer->allocatedData());
    *sceneDataPtr = _sceneData;
    
    _sceneDataDescriptorSet->updateBuffer(
        0, // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        _sceneDataBuffer.get(),
        sizeof(SceneData),
        0
    );
}

void SkySphereDelegate::initMesh()
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
    
    _models = {
        vkme::geo::Sphere::createUvSphere(
            _vulkanData,
            4.5f,
            "Sphere",
            {
                std::shared_ptr<vkme::geo::Modifier>(new vkme::geo::FlipFacesModifier()),
                std::shared_ptr<vkme::geo::Modifier>(new vkme::geo::FlipNormalsModifier())
            }
        )
    };
    
    for (auto m : _models)
    {
        m->allocateMaterialDescriptorSets(_descriptorSetAllocator.get(), _imageDescriptorLayout);
        
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

void SkySphereDelegate::drawBackground(VkCommandBuffer cmd, uint32_t currentFrame, const vkme::core::Image* depthImage)
{
    VkClearColorValue clearValue;
    float r = std::abs(std::sin(currentFrame / 90.0f)) * 0.5;
    float g = std::abs(std::sin(currentFrame / 180.0f)) * 0.5;
    float b = std::abs(std::sin(currentFrame / 120.0f)) * 0.5;
    clearValue = { { r, g, b, 1.0f } };
    auto clearRange = vkme::core::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(
        cmd,
        _drawImage->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        &clearValue, 1, &clearRange
    );
}

void SkySphereDelegate::drawGeometry(
    VkCommandBuffer cmd,
    VkImageView currentImage,
    VkExtent2D imageExtent,
    const vkme::core::Image* depthImage,
    uint32_t currentFrame,
    vkme::core::FrameResources& frameResources
) {
    
    
    for (auto& m : _models) {
        glm::mat4 modelMatrix = glm::translate(glm::mat4{ 1.0f }, glm::vec3(5.0f, 0.0f, 0.0f ));
        if (_rotateAxis == 1)
        {
            modelMatrix = glm::rotate(modelMatrix, glm::radians(float(currentFrame % 360)), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        else if (_rotateAxis == 2)
        {
            modelMatrix = glm::rotate(modelMatrix, glm::radians(float(currentFrame % 360)), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else if (_rotateAxis == 3)
        {
            modelMatrix = glm::rotate(modelMatrix, glm::radians(float(currentFrame % 360)), glm::vec3(0.0f, 0.0f, 1.0f));
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
        vkme::core::DescriptorSet* ds[] = {
            _sceneDataDescriptorSet.get()
        };
        m->draw(cmd, _pipelineLayout, ds, 1);
    }
    
    
    vkme::core::cmdEndRendering(cmd);
}
