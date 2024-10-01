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
    
    // Initialize the pipeline first, because we need the descriptor layout to initialize the
    // model descriptor sets
    initPipeline();
    
    initScene();
    
    initMesh();
    
}

void TexturesTestDelegate::initFrameResources(vkme::core::DescriptorSetAllocator * allocator)
{
    // In this example, we are storing the scene data descriptor set in the frame resources, for
    // that reason we are using this function to configure the pool in the allocator.
    // You also can store the descritor pool outside the frame resources, if you dont want to
    // use per-frame data. In this case, you don't need to use this function
    allocator->initPool(
        1000,   // Each pool can store up to 1000 descriptor set
        {  // Each descriptor set can contain
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},         // One uniform buffer. We are using it to pass the SceneData struct
            
            // Now this descriptor set is allocated from the _materialDescriptorAllocator, so
            // we don't need this kind of descriptor in the frame resources
            // { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 } // One combined image sampler. We are using it to pass the texture
        }
    );
    
    // Note: if you may store more than one type of descriptor set layout, you must specify the
    // types and count enought to store all of them. In this example we have two descriptor sets,
    // une for the uniform buffer and another for the image sampler. If you need to define
    // another descriptor set that contains two uniform buffers and three images, you must to
    // specify 2 uniform buffers and 3 images in the above structure.
    // It's also important to specify all the descriptor types that you need.
    // If the allocator pool does not meet the requirements for a descriptor set, the allocation function
    // will fail with an error. In that case, check that you are initializing the pool with all the
    // descriptor types and the number of them you need for every descriptor sets.
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
    glm::mat4 proj = glm::perspective(glm::radians(50.0f), float(newExtent.width) / float(newExtent.height), 10.0f, 0.1f);
    proj[1][1] *= -1.0f;
    proj[0][0] *= -1.0f;
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

void TexturesTestDelegate::initPipeline()
{
    vkme::factory::GraphicsPipeline plFactory(this->_vulkanData);
    
    // Load shaders
    plFactory.addShader("textures_test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("textures_test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    
    vkme::factory::DescriptorSetLayout dsFactory;
    
    // Descriptor set layout to pass the view and model matrix, and the lighting data
    // This descriptor set is stored in the frame resources, and is created every frame
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    _sceneDataDescriptorLayout = dsFactory.build(_vulkanData->device(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    
    // Descriptor set layout to pass the texture to the fragment shader. This descriptor set
    // is stored in _materialDescriptorSetAllocator, stored in this class.
    dsFactory.clear();
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _imageDescriptorLayout = dsFactory.build(_vulkanData->device(), VK_SHADER_STAGE_FRAGMENT_BIT);
    
    // Buffer range to pass the push constant, used to pass the model and normal matrixes and the
    // vertex buffer pointer
    VkPushConstantRange bufferRange = {};
    bufferRange.offset = 0;
    bufferRange.size = sizeof(vkme::geo::MeshPushConstants);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    // CONFIGURE THE PIPELINE LAYOUT: if you are using a per-surface descriptor set
    // in the layout, this must to be added always at the end of the setLayouts array.
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
    plFactory.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
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

void TexturesTestDelegate::initScene()
{
    auto viewportExtent = _vulkanData->swapchain().extent();
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f));
    // We are using a technique that consist in reversing the depth test (1 is the near plane and 0 is the far plane).
    // This technique increases the quality of the depth test.
    glm::mat4 proj = glm::perspective(glm::radians(50.0f), float(viewportExtent.width) / float(viewportExtent.height), 10.0f, 0.1f);
    proj[1][1] *= -1.0f;
    proj[0][0] *= -1.0f;
    _sceneData.view = view;
    _sceneData.proj = proj;
    _sceneData.viewProj = proj * view;
    _sceneData.ambientColor = glm::vec4{0.1f, 0.1f, 0.1f, 1.0f};
    _sceneData.sunlightColor = glm::vec4{0.9, 0.87, 0.82, 1.0f};
    _sceneData.sunlightDirection = glm::vec4{5.0, 5.0, 0.0, 1.0};
}

void TexturesTestDelegate::initMesh()
{
    // Load material data
    const std::string imagePath = vkme::PlatformTools::assetPath() + "taza_albedo.png";
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
    
    

    // Initialize the material descriptor set allocator
    _materialDescriptorSetAllocator = std::unique_ptr<vkme::core::DescriptorSetAllocator>(
        new vkme::core::DescriptorSetAllocator()
    );
    _materialDescriptorSetAllocator->init(_vulkanData);
    // Configure the descriptor set types that will hold out material
    _materialDescriptorSetAllocator->initPool(1000, {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    });
    _vulkanData->cleanupManager().push([&](VkDevice) {
        _materialDescriptorSetAllocator->clearDescriptors();
        _materialDescriptorSetAllocator->destroy();
    });
    
    
    std::string assetsPath = vkme::PlatformTools::assetPath() + "taza.obj";
    
    _models = vkme::geo::Model::loadObj(_vulkanData, assetsPath);
    
    // Allocate model material descriptor sets and initialize the descriptor set
    // data. The model contains an array to store one descriptor set for each surface,
    // you can use this descriptor set to setup the material data that you need to
    // pass to the shaders
    for (auto m : _models)
    {
        // If you call this function, you are enabling the per-surface descriptor set
        m->allocateMaterialDescriptorSets(_materialDescriptorSetAllocator.get(), _imageDescriptorLayout);
        
        // Update model material descriptor set. We are using the same image for all the surfaces
        // This function must to be executed every time we need to modify something in the
        // material properties with the descriptor set (images, uniform buffers, etc)
        // We are using the texture and sampler intialized in initScene function, and we
        // are using the same textore for every surface in the model.
        // This function does nothing if you don't previously allocate the material descriptor sets
        m->updateDescriptorSets([&](vkme::core::DescriptorSet* ds) {
            ds->updateImage(
                0,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                _textureImage->imageView(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _imageSampler
            );
            
            // If the descriptor set contains more than one binding
            // ds->beginUpdate();
            // ds->addImage(0, , ,  )
            // ds->addBuffer(1, , , , )
            // ds->addBuffer(2, , , , )
            // ds->addImage(3, , ,  )
            // ...
            // ds->endUpdate();
        });
    }
    

    
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

  // Begin update code: code executed for each scene object, before the rendering code
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
    sceneDS->updateBuffer(
        0, // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        sceneDataBuffer,
        sizeof(SceneData),
        0
    );
    
    // Update loop:
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
  // End update code
  
  // Begin rendering code: code executed after all scene elements have been updated
    
    auto colorAttachment = vkme::core::Info::attachmentInfo(currentImage, nullptr);
    auto depthAttachment = vkme::core::Info::depthAttachmentInfo(depthImage->imageView());
    auto renderInfo = vkme::core::Info::renderingInfo(imageExtent, &colorAttachment, &depthAttachment);
    
    vkme::core::cmdBeginRendering(cmd, &renderInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _transparentMaterial ? _transparentPipeline : _pipeline );
    
    VkViewport viewport = {};
    viewport.x = 0.0f; viewport.y = 0.0f;
    viewport.width = float(imageExtent.width); viewport.height = float(imageExtent.height);
    viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    
    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = imageExtent.width;
    scissor.extent.height = imageExtent.height;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    
    for (auto m : _models)
    {
        // If you are using more descriptor sets in your shader, you can pass them here.
        // If you are also using the per-surface material descriptor set it will be
        // added at the end of the ds array before binding it.
        vkme::core::DescriptorSet* ds[] = { sceneDS.get() };
        m->draw(cmd, _pipelineLayout, ds, 1);
    }
    
    
    vkme::core::cmdEndRendering(cmd);
  // End rendering code
}
