#include <RenderToCubemap.hpp>
#include <vkme/factory/GraphicsPipeline.hpp>
#include <vkme/core/Info.hpp>
#include <vkme/factory/DescriptorSetLayout.hpp>
#include <vkme/factory/Sampler.hpp>
#include <vkme/geo/Model.hpp>
#include <vkme/geo/Sphere.hpp>
#include <vkme/geo/Cube.hpp>
#include <vkme/geo/Modifiers.hpp>
#include <numbers>
#include <array>

#include <vkme/PlatformTools.hpp>

void SceneCubemap::initPipeline(vkme::VulkanData* vulkanData)
{
    vkme::factory::GraphicsPipeline plFactory(vulkanData);
    
    plFactory.addShader("cubemap_consumer_test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("cubemap_consumer_test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    vkme::factory::DescriptorSetLayout dsFactory;
    
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    sceneDataDescriptorLayout = dsFactory.build(
        vulkanData->device(), 
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    );
    
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
    plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
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
    
    // Update scene data buffer
    glm::mat4 view = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, 3.0f));
    
    sceneData.view = view;
    sceneData.proj = proj;
    sceneData.ambientColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    sceneData.sunlightColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    sceneData.sunlightDirection = glm::vec4(4.0f, 4.0f, -2.0f, 1.0f);
    
	// The scene data buffer is created in the frame resources and destroyed in the cleanup of every frame.
    // Do this when the buffer needs to be updated every frame, because the update is done in CPU time, and
    // this can cause problems when you are rendering more than one frame in flight
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
    
    _cubeMapRenderer = std::unique_ptr<vkme::tools::SphereToCubemapRenderer>(
        new vkme::tools::SphereToCubemapRenderer(_vulkanData, _descriptorSetAllocator.get())
    );
    
    auto imagePath = vkme::PlatformTools::assetPath() + "country_field_sun.jpg";
    _cubeMapRenderer->build(imagePath);
    
    auto viewportExtent = _vulkanData->swapchain().extent();
    glm::mat4 proj = glm::perspective(glm::radians(50.0f), float(viewportExtent.width) / float(viewportExtent.height), 0.1f, 10.0f);
    proj[1][1] *= -1.0f;
    proj[0][0] *= -1.0f;
    _scene.initPipeline(_vulkanData);
    _scene.initScene(_vulkanData, _descriptorSetAllocator.get(), proj);
    
    initMeshScene(_scene);
}

void RenderToCubemap::initFrameResources(vkme::core::DescriptorSetAllocator * allocator)
{
    allocator->initPool(10, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
    });
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
    
    auto proj = glm::perspective(glm::radians(50.0f), float(newExtent.width) / float(newExtent.height), 0.1f, 10.0f);
    proj[1][1] *= -1.0f;
    proj[0][0] *= -1.0f;
    _scene.sceneData.proj = proj;

    // The sceneData buffer is created in the frame resources.
}

void RenderToCubemap::cleanup()
{
    _drawImage->cleanup();
}

void RenderToCubemap::update(int32_t currentFrame, vkme::core::FrameResources& frameResources)
{
    glm::mat4 view = glm::translate(glm::mat4{ 1.0f }, glm::vec3(_cameraX, _cameraY, _cameraZ));
    _scene.sceneData.view = view;
}

VkImageLayout RenderToCubemap::draw(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    const vkme::core::Image* colorImage,
    const vkme::core::Image* depthImage,
    vkme::core::FrameResources& frameResources
) {
    using namespace vkme;

    _cubeMapRenderer->update(cmd, currentFrame);

    // Update the scene object model matrix
    std::array<glm::mat4,3> positions = {
        glm::translate(glm::mat4{1.0}, glm::vec3{-0.8, 0.5, 0.0}),
        glm::translate(glm::mat4{1.0}, glm::vec3{ 0.8, 0.5, 0.0}),
        glm::translate(glm::mat4{1.0}, glm::vec3{ 0.0, -0.7, 0.0})
    };
    auto i = 0;
    for (auto& m : _scene.models) {
        glm::mat4 modelMatrix = positions[i];
        ++i;
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
    
    drawGeometry(cmd, _drawImage->imageView(), colorImage->extent2D(), depthImage, currentFrame, frameResources, _scene);
    
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
        
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("Camera Position"))
        {
            ImGui::Columns(4, "", true);
            if (ImGui::Button("Left"))
            {
                _cameraX -= 0.1f;
            }
            ImGui::NextColumn();
            if (ImGui::Button("Right"))
            {
                _cameraX += 0.1f;
            }
            ImGui::NextColumn();
            if (ImGui::Button("Front"))
            {
                _cameraZ -= 0.1f;
            }
            ImGui::NextColumn();
            if (ImGui::Button("Back"))
            {
                _cameraZ += 0.1f;
            }
            ImGui::Columns(4, "", true);
            ImGui::NextColumn();
            if (ImGui::Button("Up"))
            {
                _cameraY -= 0.1f;
            }
            ImGui::NextColumn();
            if (ImGui::Button("Down"))
            {
                _cameraY += 0.1f;
            }
        }
        
    }
    ImGui::End();
}

void RenderToCubemap::initMeshScene(SceneCubemap& scene)
{
    scene.textureImage = std::shared_ptr<vkme::core::Image>(_cubeMapRenderer->cubeMapImage());

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(_vulkanData->device(), &samplerInfo, nullptr, &scene.imageSampler);

    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        vkDestroySampler(dev, scene.imageSampler, nullptr);
    });

    std::string assetsPath = vkme::PlatformTools::assetPath() + "taza.obj";

    auto basicMesh = vkme::geo::Model::loadObj(_vulkanData, assetsPath, {
		std::shared_ptr<vkme::geo::Modifier>(new vkme::geo::ScaleModifier(0.5f))
    });

    scene.models = {
        vkme::geo::Cube::createCube(_vulkanData, 1.0f),
        vkme::geo::Sphere::createUvSphere(_vulkanData, 0.25f)
    };

    for (auto m : basicMesh)
    {
        scene.models.push_back(m);
    }
    
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
    auto sceneDataBuffer = vkme::core::Buffer::createAllocatedBuffer(
        _vulkanData,
        sizeof(SceneDataCubemap),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

	SceneDataCubemap* sceneDataPtr = reinterpret_cast<SceneDataCubemap*>(sceneDataBuffer->allocatedData());
	*sceneDataPtr = _scene.sceneData;

	frameResources.cleanupManager.push([&, sceneDataBuffer](VkDevice) {
		sceneDataBuffer->cleanup();
        delete sceneDataBuffer;
	});

    auto sceneDS = std::unique_ptr<vkme::core::DescriptorSet>(
        frameResources.descriptorAllocator->allocate(_scene.sceneDataDescriptorLayout)
    );
	sceneDS->updateBuffer(
		0,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		sceneDataBuffer,
		sizeof(SceneDataCubemap),
		0
	);

    auto colorAttachment = vkme::core::Info::attachmentInfo(currentImage, nullptr);
    auto depthAttachment = vkme::core::Info::depthAttachmentInfo(depthImage->imageView(), 1.0);
    auto renderInfo = vkme::core::Info::renderingInfo(imageExtent, &colorAttachment, &depthAttachment);
    vkme::core::cmdBeginRendering(cmd, &renderInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.pipeline);

    cmdSetDefaultViewportAndScissor(cmd, imageExtent);

    for (auto m : scene.models)
    {
        vkme::core::DescriptorSet* ds[] = {
			sceneDS.get()
        };
        m->draw(cmd, scene.pipelineLayout, ds, 1);
    }
    vkme::core::cmdEndRendering(cmd);    
}
