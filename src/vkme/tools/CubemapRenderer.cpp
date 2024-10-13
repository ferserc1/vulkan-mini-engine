#include <vkme/tools/CubemapRenderer.hpp>

namespace vkme::tools {

CubemapRenderer::CubemapRenderer(VulkanData * vulkanData, vkme::core::DescriptorSetAllocator * descriptorSetAllocator)
: _vulkanData(vulkanData), _descriptorSetAllocator(descriptorSetAllocator) {
    
}

void CubemapRenderer::build(
    const std::string& imagePath,
    const std::string& vertexShaderPath,
    const std::string& fragmentShaderPath,
    VkExtent2D cubeImageSize
) {

}

void CubemapRenderer::update(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
    
}

void CubemapRenderer::cleanup() {
    
}

void CubemapRenderer::initImages() {
    
}

void CubemapRenderer::initPipeline() {
    
}

void CubemapRenderer::initGeometry() {
    
}

}
