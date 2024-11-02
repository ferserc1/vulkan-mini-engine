#include <vkme/tools/IrradianceCubemapRenderer.hpp>
#include <vkme/factory/DescriptorSetLayout.hpp>

namespace vkme::tools {

IrradianceCubemapRenderer::IrradianceCubemapRenderer(VulkanData * vulkanData, vkme::core::DescriptorSetAllocator * allocator)
    :CubemapRenderer(vulkanData, allocator)
{
    
}

void IrradianceCubemapRenderer::build(
    std::shared_ptr<vkme::core::Image> inputSkyBox,
    VkExtent2D cubeImageSize
) {
	CubemapRenderer::build(
        inputSkyBox, 
        "cubemap_renderer.vert.spv",
        "irradiance_map_skybox.frag.spv",
        cubeImageSize
    );
}

void IrradianceCubemapRenderer::update(
    VkCommandBuffer commandBuffer,
    uint32_t currentFrame,
    vkme::core::FrameResources& frameResources
) {
    CubemapRenderer::update(commandBuffer, currentFrame);
}


}

