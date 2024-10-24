#pragma once

#include <vkme/tools/CubemapRenderer.hpp>

namespace vkme::tools {

class SpecularReflectionCubemapRenderer : public CubemapRenderer {
public:
    SpecularReflectionCubemapRenderer(VulkanData *, vkme::core::DescriptorSetAllocator *);

    static void getFrameResourcesRequirements(std::vector<vkme::core::DescriptorSetAllocator::PoolSizeRatio>& ratios)
    {
        ratios.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 });
    }

    void build(
        std::shared_ptr<vkme::core::Image> inputSkyBox,
        VkExtent2D cubeImageSize = { 1024, 1024 }
    );

    inline void setRoughness(float roughness) { _roughness = roughness; }
    inline float roughness() const { return _roughness; }

	inline void setSampleCount(int sampleCount) { _sampleCount = sampleCount; }
	inline int sampleCount() const { return _sampleCount; }

    void update(VkCommandBuffer commandBuffer, uint32_t currentFrame, vkme::core::FrameResources& frameResources);

protected:
    float _roughness;
    int _sampleCount;

    struct SpecularReflectionData {
        float roughness;
        int sampleCount;
    };

    VkDescriptorSetLayout _specularReflectionDSLayout;
};

}