#include <vkme/tools/SpecularReflectionCubemapRenderer.hpp>
#include <vkme/factory/DescriptorSetLayout.hpp>

namespace vkme::tools {

SpecularReflectionCubemapRenderer::SpecularReflectionCubemapRenderer(VulkanData * vulkanData, vkme::core::DescriptorSetAllocator * allocator)
    :CubemapRenderer(vulkanData, allocator), _roughness(0.0f), _sampleCount(256)
{
    
}

void SpecularReflectionCubemapRenderer::build(
    std::shared_ptr<vkme::core::Image> inputSkyBox,
    VkExtent2D cubeImageSize
) {
    vkme::factory::DescriptorSetLayout dsFactory;
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    _specularReflectionDSLayout = dsFactory.build(_vulkanData->device(), VK_SHADER_STAGE_FRAGMENT_BIT);

	CubemapRenderer::build(
        inputSkyBox, 
        "cubemap_renderer.vert.spv",
        "specular_reflection_skybox.frag.spv",
        cubeImageSize,
        _specularReflectionDSLayout
    );

    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        vkDestroyDescriptorSetLayout(dev, _specularReflectionDSLayout, nullptr);
    });
}

void SpecularReflectionCubemapRenderer::update(
    VkCommandBuffer commandBuffer,
    uint32_t currentFrame,
    vkme::core::FrameResources& frameResources
) {
    auto specularReflectionBuffer = vkme::core::Buffer::createAllocatedBuffer(
        _vulkanData,
        sizeof(SpecularReflectionData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

    auto specularReflectionPtr = reinterpret_cast<SpecularReflectionData*>(specularReflectionBuffer->allocatedData());
    specularReflectionPtr->roughness = _roughness;
	specularReflectionPtr->sampleCount = _sampleCount;
    auto specularReflectionDS = std::unique_ptr<vkme::core::DescriptorSet>(
        frameResources.descriptorAllocator->allocate(_specularReflectionDSLayout)
    );

    specularReflectionDS->updateBuffer(
        0,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        specularReflectionBuffer,
        sizeof(SpecularReflectionData),
        0
    );

    frameResources.cleanupManager.push([&, specularReflectionBuffer](VkDevice) {
        specularReflectionBuffer->cleanup();
        delete specularReflectionBuffer;
    });

    CubemapRenderer::update(commandBuffer, currentFrame, specularReflectionDS.get());
}


}

