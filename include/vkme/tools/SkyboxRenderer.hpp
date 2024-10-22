#pragma once

#include <vkme/VulkanData.hpp>
#include <vkme/geo/Model.hpp>
#include <vkme/core/DescriptorSetAllocator.hpp>
#include <memory>
#include <vector>

namespace vkme::tools {

class SkyboxRenderer {
public:
    SkyboxRenderer(VulkanData*, core::DescriptorSetAllocator*);
    
    static void getFrameResourcesRequirements(std::vector<core::DescriptorSetAllocator::PoolSizeRatio>& requiredRatios);
    
    void init(std::shared_ptr<core::Image>&& skyImage);
    
    void update(const glm::mat4& view, const glm::mat4& proj);
    
    void draw(VkCommandBuffer cmd, uint32_t currentFrame, vkme::core::FrameResources& frameResources);
    
protected:
    VulkanData* _vulkanData;
    core::DescriptorSetAllocator* _descriptorSetAllocator;
    
    std::shared_ptr<vkme::core::Image> _skyImage;

    std::shared_ptr<vkme::geo::Model> _skyCube;
    VkPipelineLayout _pipelineLayout;
    VkPipeline _pipeline;
    VkDescriptorSetLayout _uniformBufferDSLayout;
    VkDescriptorSetLayout _inputImageDSLayout;
    VkSampler _imageSampler;
    struct SkyData {
        glm::mat4 view;
        glm::mat4 proj;
    };
    SkyData _skyData;
};

}
