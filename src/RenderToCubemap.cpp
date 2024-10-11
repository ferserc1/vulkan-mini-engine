#include <RenderToCubemap.hpp>
#include <vkme/factory/GraphicsPipeline.hpp>
#include <vkme/core/Info.hpp>
#include <vkme/factory/DescriptorSetLayout.hpp>
#include <vkme/geo/Sphere.hpp>
#include <vkme/geo/Cube.hpp>
#include <vkme/geo/Modifiers.hpp>
#include <numbers>

#include <vkme/PlatformTools.hpp>

void SceneCubemap::initPipeline(vkme::VulkanData* vulkanData, uint32_t viewLayers)
{
    vkme::factory::GraphicsPipeline plFactory(vulkanData);
    
    if (viewLayers == 6)
    {
        plFactory.addShader("cubemap_test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        plFactory.addShader("cubemap_test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    else {
        plFactory.addShader("cubemap_consumer_test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        plFactory.addShader("cubemap_consumer_test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }
   
    
    vkme::factory::DescriptorSetLayout dsFactory;
    
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    sceneDataDescriptorLayout = dsFactory.build(vulkanData->device(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    
    dsFactory.clear();
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    imageDescriptorLayout = dsFactory.build(vulkanData->device(), VK_SHADER_STAGE_FRAGMENT_BIT);
    
    VkPushConstantRange bufferRange = {};
    bufferRange.offset = 0;
    bufferRange.size = sizeof(vkme::geo::MeshPushConstants);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    auto layoutInfo = vkme::core::Info::pipelineLayoutInfo();
    layoutInfo.pPushConstantRanges = &bufferRange;
    layoutInfo.pushConstantRangeCount = 1;
    VkDescriptorSetLayout layouts[] = {
        sceneDataDescriptorLayout,
        imageDescriptorLayout
    };
    layoutInfo.pSetLayouts = layouts;
    layoutInfo.setLayoutCount = 2;
    
    VK_ASSERT(vkCreatePipelineLayout(vulkanData->device(), &layoutInfo, nullptr, &pipelineLayout));
    
    plFactory.setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
    plFactory.setDepthFormat(vulkanData->swapchain().depthImageFormat());
    if (viewLayers == 0)
    {
        plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    }
    else
    {
        plFactory.disableDepthtest();
    }
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(true, VK_FRONT_FACE_CLOCKWISE);
    
    pipeline = plFactory.build(pipelineLayout);
    
    vulkanData->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, pipeline, nullptr);
        vkDestroyPipelineLayout(dev, pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, sceneDataDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, imageDescriptorLayout, nullptr);
    });
}

void SceneCubemap::initScene(vkme::VulkanData* vulkanData, vkme::core::DescriptorSetAllocator * dsAllocator, const glm::mat4& proj)
{
    // Allocate scene data descriptor set
    sceneDataDescriptorSet = std::unique_ptr<vkme::core::DescriptorSet>(
        dsAllocator->allocate(sceneDataDescriptorLayout)
    );
    
    // Update scene data buffer
    glm::mat4 view = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, 3.0f));
    
    sceneData.view[0] = view;
    //sceneData.view[0] = glm::rotate(glm::mat4{ 1.0f }, std::numbers::pi_v<float>, glm::vec3(0.0, 1.0, 0.0));
	sceneData.view[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    sceneData.view[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    sceneData.view[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    sceneData.view[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    sceneData.view[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    sceneData.proj = proj;
    sceneData.ambientColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    sceneData.sunlightColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    sceneData.sunlightDirection = glm::vec4(4.0f, 4.0f, -2.0f, 1.0f);
    
    sceneDataBuffer = std::unique_ptr<vkme::core::Buffer>(vkme::core::Buffer::createAllocatedBuffer(
        vulkanData,
        sizeof(SceneDataCubemap),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    ));
    
    vulkanData->cleanupManager().push([&](VkDevice) {
        sceneDataBuffer->cleanup();
    });
    
    SceneDataCubemap* sceneDataPtr = reinterpret_cast<SceneDataCubemap*>(sceneDataBuffer->allocatedData());
    *sceneDataPtr = sceneData;
    
    sceneDataDescriptorSet->updateBuffer(
        0, // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        sceneDataBuffer.get(),
        sizeof(SceneDataCubemap),
        0
    );
}
    
void RenderToCubemap::init(vkme::VulkanData * vulkanData)
{
    _vulkanData = vulkanData;

	// The second scene will be rendered to this image
    _drawImage = std::shared_ptr<vkme::core::Image>(vkme::core::Image::createAllocatedImage(
        vulkanData,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        vulkanData->swapchain().extent(),
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    ));

	// The first image will be rendered into this image
    _rttImage = std::shared_ptr<vkme::core::Image>(vkme::core::Image::createAllocatedImage(
        vulkanData,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VkExtent2D(1024, 1024),
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        6
    ));

	// Create image views for each face
    // The image view at the Image class corresponds with the cubemap image view
	
    VkImageView imgView;
	VkImageViewCreateInfo viewInfo = vkme::core::Info::imageViewCreateInfo(VK_FORMAT_R16G16B16A16_SFLOAT, _rttImage->image(), VK_IMAGE_ASPECT_COLOR_BIT);
	viewInfo.subresourceRange.layerCount = 1;

	_rttImageViews.resize(6);
    for (int i = 0; i < 6; ++i)
    {
        viewInfo.subresourceRange.baseArrayLayer = i;
	    vkCreateImageView(vulkanData->device(), &viewInfo, nullptr, &imgView);
	    _rttImageViews[i] = imgView;
    }

    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        // Skip the first image view because this is managed by the Image class
        for (int i = 0; i < 6; ++i)
        {
            vkDestroyImageView(dev, _rttImageViews[i], nullptr);
        }
    });

    _rttDepthImage = std::shared_ptr<vkme::core::Image>(vkme::core::Image::createAllocatedImage(
        vulkanData,
        vulkanData->swapchain().depthImageFormat(),
        VkExtent2D(1024, 1024),
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    ));
    
    vulkanData->cleanupManager().push([this](VkDevice) {
        this->cleanup();
    });
    
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
    
    initScenes();
    
    initMeshScene1(_scene1);
	initMeshScene2(_scene2);
}

void RenderToCubemap::initFrameResources(vkme::core::DescriptorSetAllocator * allocator)
{
}

void RenderToCubemap::swapchainResized(VkExtent2D newExtent)
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
    
    glm::mat4 proj = glm::perspective(glm::radians(50.0f), 1.0f, 0.1f, 10.0f);
    proj[1][1] *= -1.0f;
    proj[0][0] *= -1.0f;
    _scene1.sceneData.proj = proj;

    SceneDataCubemap* sceneDataPtr = reinterpret_cast<SceneDataCubemap*>(_scene1.sceneDataBuffer->allocatedData());
    *sceneDataPtr = _scene1.sceneData;
    
    _scene1.sceneDataDescriptorSet->updateBuffer(
        0, // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        _scene1.sceneDataBuffer.get(),
        sizeof(SceneDataCubemap),
        0
    );

    proj = glm::perspective(glm::radians(50.0f), float(newExtent.width) / float(newExtent.height), 0.1f, 10.0f);
    proj[1][1] *= -1.0f;
    proj[0][0] *= -1.0f;
    _scene2.sceneData.proj = proj;

    sceneDataPtr = reinterpret_cast<SceneDataCubemap*>(_scene2.sceneDataBuffer->allocatedData());
    *sceneDataPtr = _scene2.sceneData;

    _scene2.sceneDataDescriptorSet->updateBuffer(
        0,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        _scene2.sceneDataBuffer.get(),
        sizeof(SceneDataCubemap),
        0
    );
}

void RenderToCubemap::cleanup()
{
    _drawImage->cleanup();
	_rttImage->cleanup();
	_rttDepthImage->cleanup();
}

VkImageLayout RenderToCubemap::draw(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const vkme::core::Image* colorImage,
    const vkme::core::Image* depthImage,
    vkme::core::FrameResources& frameResources
) {
    using namespace vkme;

    // Update the scene 1 object model matrix
    for (auto& m : _scene1.models) {
        glm::mat4 modelMatrix = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, 0.0f));
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

    // Update the scene 2 object model matrix
    for (auto& m : _scene2.models) {
        glm::mat4 modelMatrix = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, 0.0f));
        if (_rotateAxisCube == 1)
        {
            modelMatrix = glm::rotate(modelMatrix, glm::radians(float(currentFrame % 360)), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        else if (_rotateAxisCube == 2)
        {
            modelMatrix = glm::rotate(modelMatrix, glm::radians(float(currentFrame % 360)), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else if (_rotateAxisCube == 3)
        {
            modelMatrix = glm::rotate(modelMatrix, glm::radians(float(currentFrame % 360)), glm::vec3(0.0f, 0.0f, 1.0f));
        }
        m->setModelMatrix(modelMatrix);
    }




    // Render first scene to the texture
	core::Image::cmdTransitionImage(
		cmd,
		_rttImage->image(),
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL
	);

    core::Image::cmdTransitionImage(
        cmd,
        _rttDepthImage->image(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    // We pass 120 more frames because the background colour is calculated from there, so we have a different background colour for each render pass.
    drawBackground(cmd, currentFrame + 120, _rttImage.get());

    core::Image::cmdTransitionImage(
        cmd,
        _rttDepthImage->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    );

    core::Image::cmdTransitionImage(
        cmd,
        _rttImage->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    drawGeometry(cmd, _rttImage->imageView(), _rttImage->extent2D(), _rttDepthImage.get(), currentFrame, frameResources, _scene1, 6);

    core::Image::cmdTransitionImage(
        cmd,
        _rttImage->image(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );


    
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
    
    drawBackground(cmd, currentFrame, _drawImage.get());
    
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
    
    drawGeometry(cmd, _drawImage->imageView(), colorImage->extent2D(), depthImage, currentFrame, frameResources, _scene2);
    
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

void RenderToCubemap::drawUI()
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


        if (_rotateAxisCube == 0)
        {
            ImGui::Text("No rotation");
        }
        else if (_rotateAxisCube == 1)
        {
            ImGui::Text("Rotate X");
        }
        else if (_rotateAxisCube == 2)
        {
            ImGui::Text("Rotate Y");
        }
        else if (_rotateAxisCube == 3)
        {
            ImGui::Text("Rotate Z");
        }

        ImGui::SliderInt("Cube Rotation axis", reinterpret_cast<int*>(&_rotateAxisCube), 0, 3);
    }
    ImGui::End();
}

void RenderToCubemap::initScenes()
{
	auto viewportExtent = _vulkanData->swapchain().extent();
    glm::mat4 proj = glm::perspective(glm::radians(50.0f), 1.0f, 0.1f, 10.0f);
    proj[1][1] *= -1.0f;
    proj[0][0] *= -1.0f;
    _scene1.initPipeline(_vulkanData, 6);
    _scene1.initScene(_vulkanData, _descriptorSetAllocator.get(), proj);

    proj = glm::perspective(glm::radians(50.0f), float(viewportExtent.width) / float(viewportExtent.height), 0.1f, 10.0f);
    proj[1][1] *= -1.0f;
    proj[0][0] *= -1.0f;
    _scene2.initPipeline(_vulkanData);
	_scene2.initScene(_vulkanData, _descriptorSetAllocator.get(), proj);
}

void RenderToCubemap::initMeshScene1(SceneCubemap& scene)
{
    const std::string imagePath = vkme::PlatformTools::assetPath() + "country_field_sun.jpg";
    scene.textureImage = std::shared_ptr<vkme::core::Image>(
        vkme::core::Image::loadImage(_vulkanData, imagePath)
    );
    
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(_vulkanData->device(), &samplerInfo, nullptr, &scene.imageSampler);
    
    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        scene.textureImage->cleanup();
        vkDestroySampler(dev, scene.imageSampler, nullptr);
    });
    
    scene.models = {
        vkme::geo::Sphere::createUvSphere(
            _vulkanData,
            6.0f,
            "Sphere",
            {
                std::shared_ptr<vkme::geo::Modifier>(new vkme::geo::FlipFacesModifier()),
				std::shared_ptr<vkme::geo::FlipNormalsModifier>(new vkme::geo::FlipNormalsModifier())
            }
        )
    };
    
    for (auto m : scene.models)
    {
        m->allocateMaterialDescriptorSets(_descriptorSetAllocator.get(), scene.imageDescriptorLayout);
        
        m->updateDescriptorSets([&](vkme::core::DescriptorSet* ds) {
            ds->updateImage(
                0,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                scene.textureImage->imageView(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                scene.imageSampler
            );
        });
    }
    
    _vulkanData->cleanupManager().push([&](VkDevice) {
        for (auto m : scene.models) {
            m->cleanup();
        }
    });
}

void RenderToCubemap::initMeshScene2(SceneCubemap& scene)
{
    scene.textureImage = std::shared_ptr<vkme::core::Image>(_rttImage);


    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(_vulkanData->device(), &samplerInfo, nullptr, &scene.imageSampler);

    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        vkDestroySampler(dev, scene.imageSampler, nullptr);
    });

    scene.models = {
        vkme::geo::Cube::createCube(
            _vulkanData,
            1.0f,
            "Cube"
        )
    };

    for (auto m : scene.models)
    {
        m->allocateMaterialDescriptorSets(_descriptorSetAllocator.get(), scene.imageDescriptorLayout);

        m->updateDescriptorSets([&](vkme::core::DescriptorSet* ds) {
            ds->updateImage(
                0,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                scene.textureImage->imageView(),
                //scene.textureImage->imageView(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                scene.imageSampler
            );
        });
    }

    _vulkanData->cleanupManager().push([&](VkDevice) {
        for (auto m : scene.models) {
            m->cleanup();
        }
        });
}

void RenderToCubemap::drawBackground(VkCommandBuffer cmd, uint32_t currentFrame, const vkme::core::Image* image)
{
    VkClearColorValue clearValue;
    float r = std::abs(std::sin(currentFrame / 90.0f)) * 0.5f;
    float g = std::abs(std::sin(currentFrame / 180.0f)) * 0.5f;
    float b = std::abs(std::sin(currentFrame / 120.0f)) * 0.5f;
    clearValue = { { r, g, b, 1.0f } };
    auto clearRange = vkme::core::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(
        cmd,
        image->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        &clearValue, 1, &clearRange
    );
}

void RenderToCubemap::drawGeometry(
    VkCommandBuffer cmd,
    VkImageView currentImage,
    VkExtent2D imageExtent,
    const vkme::core::Image* depthImage,
    uint32_t currentFrame,
    vkme::core::FrameResources& frameResources,
    SceneCubemap& scene,
    uint32_t layerCount
) {
    if (layerCount > 1)
    {
        // As I can't get VK_KHR_multiview to work, we simply do it by hand. We make six render passes, one for each face of the cube.

        for (int i = 0; i < 6; ++i)
        {
			auto view = _rttImageViews[i];
            auto colorAttachment = vkme::core::Info::attachmentInfo(view, nullptr);
            auto renderInfo = vkme::core::Info::renderingInfo(imageExtent, &colorAttachment, nullptr);
            vkme::core::cmdBeginRendering(cmd, &renderInfo);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.pipeline);

            cmdSetDefaultViewportAndScissor(cmd, imageExtent);

            for (auto m : scene.models)
            {
                vkme::core::DescriptorSet* ds[] = {
                    scene.sceneDataDescriptorSet.get()
                };
                // He añadido un parámetro entero en las push constants para indicar el índice de la cara a renderizar
                m->draw(cmd, scene.pipelineLayout, ds, 1, i);
            }
            vkme::core::cmdEndRendering(cmd);
        }
    }
    else {
        auto colorAttachment = vkme::core::Info::attachmentInfo(currentImage, nullptr);
        auto depthAttachment = vkme::core::Info::depthAttachmentInfo(depthImage->imageView(), 1.0);
        auto renderInfo = vkme::core::Info::renderingInfo(imageExtent, &colorAttachment, &depthAttachment);
        vkme::core::cmdBeginRendering(cmd, &renderInfo);


        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.pipeline);

        cmdSetDefaultViewportAndScissor(cmd, imageExtent);

        for (auto m : scene.models)
        {
            vkme::core::DescriptorSet* ds[] = {
                scene.sceneDataDescriptorSet.get()
            };
            m->draw(cmd, scene.pipelineLayout, ds, 1);
        }
        vkme::core::cmdEndRendering(cmd);
    }
    
    
}
