#pragma once

#include <vkme/tools/CubemapRenderer.hpp>

namespace vkme::tools {

class IrradianceCubemapRenderer : public CubemapRenderer {
public:
    IrradianceCubemapRenderer(VulkanData *, vkme::core::DescriptorSetAllocator *);

    static void getFrameResourcesRequirements(std::vector<vkme::core::DescriptorSetAllocator::PoolSizeRatio>& ratios)
    {
        ratios.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 });
    }

    void build(
        std::shared_ptr<vkme::core::Image> inputSkyBox,
        VkExtent2D cubeImageSize = { 1024, 1024 }
    );

    void update(VkCommandBuffer commandBuffer, uint32_t currentFrame, vkme::core::FrameResources& frameResources);
};

}
