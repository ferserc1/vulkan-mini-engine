#pragma once

#include <vkme/tools/CubemapRenderer.hpp>
#include <vkme/core/Info.hpp>
#include <vkme/factory/GraphicsPipeline.hpp>
#include <vkme/factory/DescriptorSetLayout.hpp>
#include <vkme/factory/Sampler.hpp>
#include <vkme/geo/Cube.hpp>
#include <array>

namespace vkme {
namespace tools {

CubemapRenderer::CubemapRenderer(VulkanData * vulkanData, vkme::core::DescriptorSetAllocator * dsLayout)
    : _vulkanData{ vulkanData }, _descriptorSetAllocator{ dsLayout }
{
}
    
void CubemapRenderer::build(
    std::shared_ptr<vkme::core::Image> inputSkybox,
    const std::string& vertexShaderFile,
    const std::string& fragmentShaderFile,
    VkExtent2D cubeImageSize,
    VkDescriptorSetLayout customLayout
) {
    _inputSkybox = inputSkybox;
    
    initImages(cubeImageSize);
    
    vkme::factory::Sampler samplerFactory(_vulkanData);
    _skyImageSampler = samplerFactory.build(
        VK_FILTER_LINEAR,
        VK_FILTER_LINEAR
    );
    
    vkme::factory::DescriptorSetLayout dsFactory;
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _skyImageDescriptorSetLayout = dsFactory.build(
        _vulkanData->device(),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );
    
    _vulkanData->cleanupManager().push([&](VkDevice dev) {
		vkDestroyDescriptorSetLayout(dev, _skyImageDescriptorSetLayout, nullptr);
	});
    
    _skyImageDescriptorSet = std::unique_ptr<vkme::core::DescriptorSet>(
        _descriptorSetAllocator->allocate(_skyImageDescriptorSetLayout)
    );
    _skyImageDescriptorSet->updateImage(
        0,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        _inputSkybox->imageView(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        _skyImageSampler
    );
    
    initPipeline(vertexShaderFile, fragmentShaderFile, customLayout);
    initGeometry();
}

void CubemapRenderer::update(VkCommandBuffer cmd, uint32_t currentFrame, vkme::core::DescriptorSet* customSet)
{
    vkme::core::Image::cmdTransitionImage(
        cmd,
        _cubeMapImage->image(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL
    );

    VkClearColorValue clearValue;
    clearValue = { { 0.5, 0.5, 0.5, 1.0f } };
    auto clearRange = vkme::core::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(
        cmd,
        _cubeMapImage->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        &clearValue, 1, &clearRange
    );

    vkme::core::Image::cmdTransitionImage(
        cmd,
        _cubeMapImage->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    // Draw geometry
    for (auto i = 0; i < 6; ++i) {
		// Draw each face of the cube map in a separate render pass
		auto view = _cubeMapImageViews[i];
		auto colorAttachment = vkme::core::Info::attachmentInfo(view, nullptr);
		auto renderInfo = vkme::core::Info::renderingInfo(_cubeMapImage->extent2D(), &colorAttachment, nullptr);
		vkme::core::cmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

        auto viewportExtent = _cubeMapImage->extent2D();
        VkViewport viewport = {};
        viewport.x = 0; viewport.y = 0;
        viewport.width = float(viewportExtent.width);
        viewport.height = float(viewportExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = viewportExtent;
        vkCmdSetScissor(cmd, 0, 1, &scissor);
       
        // Draw the sky sphere
        auto meshBuffers = _cube->meshBuffers();
        SkySpherePushConstant pushConstants;
        pushConstants.currentFace = i;
        pushConstants.vertexBufferAddress = meshBuffers->vertexBufferAddress;

        vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SkySpherePushConstant), &pushConstants);
        vkCmdBindIndexBuffer(cmd, meshBuffers->indexBuffer->buffer(), 0, VK_INDEX_TYPE_UINT32);

        // The sphere has only one surface
        auto surface = _cube->surfaces()[0];
       
        std::vector<VkDescriptorSet> sets = {
            _projectionDataDescriptorSet->descriptorSet(),
            _skyImageDescriptorSet->descriptorSet()
        };
        if (customSet != nullptr) {
            sets.push_back(customSet->descriptorSet());
        }

        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _pipelineLayout, 0,
            uint32_t(sets.size()),
            sets.data(),
            0, nullptr
        );

        vkCmdDrawIndexed(cmd, surface.indexCount, 1, surface.startIndex, 0, 0);
        
		vkme::core::cmdEndRendering(cmd);
    }

	vkme::core::Image::cmdTransitionImage(
		cmd,
		_cubeMapImage->image(),
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
}

void CubemapRenderer::initImages(VkExtent2D extent)
{
    // Cube map image
    // This are the image views used to render the cubemap
    _cubeMapImage = std::shared_ptr<vkme::core::Image>(vkme::core::Image::createAllocatedImage(
        _vulkanData,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        extent,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        6   // 6 layers. When specify this parameter, the image is created as a cube map compatible image with 6 layers, and the image view is created as a cube map image view
    ));

    // Create image views for each face. We'll use these image views to render to the cube map image
    auto viewInfo = vkme::core::Info::imageViewCreateInfo(
        VK_FORMAT_R16G16B16A16_SFLOAT,
        _cubeMapImage->image(),
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    VkImageView imgView;
    for (int i = 0; i < 6; ++i)
    {
        viewInfo.subresourceRange.baseArrayLayer = i;
        vkCreateImageView(_vulkanData->device(), &viewInfo, nullptr, &imgView);
        _cubeMapImageViews[i] = imgView;
    }

    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        for (int i = 0; i < 6; ++i)
        {
            vkDestroyImageView(dev, _cubeMapImageViews[i], nullptr);
        }

		_cubeMapImage->cleanup();
    });
}

void CubemapRenderer::initPipeline(
    const std::string& vshaderFile,
    const std::string& fshaderFile,
    VkDescriptorSetLayout customLayout)
{
    vkme::factory::GraphicsPipeline plFactory(_vulkanData);

	plFactory.addShader(vshaderFile, VK_SHADER_STAGE_VERTEX_BIT);
	plFactory.addShader(fshaderFile, VK_SHADER_STAGE_FRAGMENT_BIT);

    vkme::factory::DescriptorSetLayout dsFactory;

	dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	_projectionDataDescriptorSetLayout = dsFactory.build(_vulkanData->device(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	
    VkPushConstantRange bufferRange = {};
    bufferRange.offset = 0;
    bufferRange.size = sizeof(SkySpherePushConstant);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	auto layoutInfo = vkme::core::Info::pipelineLayoutInfo();
	layoutInfo.pPushConstantRanges = &bufferRange;
	layoutInfo.pushConstantRangeCount = 1;

    std::vector<VkDescriptorSetLayout> layouts = {
        _projectionDataDescriptorSetLayout,
        _skyImageDescriptorSetLayout // Created in updateImage
    };
    if (customLayout != VK_NULL_HANDLE)
    {
        layouts.push_back(customLayout);
    }
	layoutInfo.pSetLayouts = layouts.data();
    layoutInfo.setLayoutCount = uint32_t(layouts.size());

	VK_ASSERT(vkCreatePipelineLayout(_vulkanData->device(), &layoutInfo, nullptr, &_pipelineLayout));

    plFactory.setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
	plFactory.disableDepthtest();
	plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	plFactory.setCullMode(true, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	_pipeline = plFactory.build(_pipelineLayout);

	_vulkanData->cleanupManager().push([&](VkDevice dev) {
		vkDestroyPipeline(dev, _pipeline, nullptr);
		vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(dev, _projectionDataDescriptorSetLayout, nullptr);
	});
}

void CubemapRenderer::initGeometry()
{
    _projectionDataDescriptorSet = std::unique_ptr<vkme::core::DescriptorSet>(
        _descriptorSetAllocator->allocate(_projectionDataDescriptorSetLayout)
    );

	_projectionData.view[0] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _projectionData.view[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _projectionData.view[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    _projectionData.view[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f));
    _projectionData.view[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, 0.0f,-1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _projectionData.view[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	_projectionData.proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);
    _projectionData.proj[1][1] *= -1.0f;
    _projectionData.proj[0][0] *= -1.0f;
	_projectionDataBuffer = std::unique_ptr<vkme::core::Buffer>(vkme::core::Buffer::createAllocatedBuffer(
		_vulkanData,
		sizeof(ProjectionData),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	));

	_vulkanData->cleanupManager().push([&](VkDevice) {
		_projectionDataBuffer->cleanup();
	});

	ProjectionData* projectionDataPtr = reinterpret_cast<ProjectionData*>(_projectionDataBuffer->allocatedData());
	*projectionDataPtr = _projectionData;

	_projectionDataDescriptorSet->updateBuffer(
		0, // binding
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		_projectionDataBuffer.get(),
		sizeof(ProjectionData),
		0
	);

	_vulkanData->cleanupManager().push([&](VkDevice dev) {
		vkDestroySampler(dev, _skyImageSampler, nullptr);
	});

	_cube = vkme::geo::Cube::createCube(
		_vulkanData,
		10.0f,
		"Sky Box"
	);
	
	_vulkanData->cleanupManager().push([&](VkDevice) {
		_cube->cleanup();
	});
}
    
}
}
