#pragma once

#include <vkme/VulkanData.hpp>
#include <vkme/core/Buffer.hpp>
#include <vkme/core/DescriptorSet.hpp>
#include <vkme/core/DescriptorSetAllocator.hpp>
#include <vkme/core/Image.hpp>
#include <vkme/geo/Model.hpp>

#include <vector>
#include <memory>


namespace vkme {
namespace tools {

class CubemapRenderer {
public:
    CubemapRenderer(VulkanData *, vkme::core::DescriptorSetAllocator *);

    void build(
        std::shared_ptr<vkme::core::Image> inputSkyBox,
        const std::string& vertexShaderFile = "cubemap_renderer.vert.spv",
        const std::string& fragmentShaderFile = "skybox.frag.spv",
        VkExtent2D cubeImageSize = { 1024, 1024 },
        VkDescriptorSetLayout customLayout = VK_NULL_HANDLE
    );

    void update(VkCommandBuffer commandBuffer, uint32_t currentFrame, vkme::core::DescriptorSet* customSet = nullptr);

    std::shared_ptr<vkme::core::Image> cubeMapImage() { return _cubeMapImage; }

protected:
    VulkanData * _vulkanData;
    vkme::core::DescriptorSetAllocator * _descriptorSetAllocator;

    std::shared_ptr<vkme::geo::Model> _cube;

    struct SkySpherePushConstant {
        VkDeviceAddress vertexBufferAddress;
        int currentFace;
    };

    struct ProjectionData {
        glm::mat4 view[6];
        glm::mat4 proj;
    };

    VkPipelineLayout _pipelineLayout;
    VkPipeline _pipeline;
    std::unique_ptr<vkme::core::Buffer> _projectionDataBuffer;
    std::unique_ptr<vkme::core::DescriptorSet> _projectionDataDescriptorSet;
    VkDescriptorSetLayout _projectionDataDescriptorSetLayout;
    ProjectionData _projectionData;

    std::shared_ptr<vkme::core::Image> _inputSkybox;
    VkDescriptorSetLayout _skyImageDescriptorSetLayout;
    std::unique_ptr<vkme::core::DescriptorSet> _skyImageDescriptorSet;
    VkSampler _skyImageSampler;

    std::shared_ptr<vkme::core::Image> _cubeMapImage;
    VkImageView _cubeMapImageViews[6];

    void initImages(VkExtent2D);
    void initPipeline(
        const std::string& vshaderFile,
        const std::string& fshaderFile,
        VkDescriptorSetLayout customLayout
    );
    void initGeometry();
};

}
}
