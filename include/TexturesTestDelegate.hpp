#pragma once

#include <vkme/core/Image.hpp>
#include <vkme/VulkanData.hpp>
#include <vkme/DrawLoop.hpp>
#include <vkme/UserInterface.hpp>
#include <vkme/geo/mesh_data.hpp>
#include <vkme/geo/Model.hpp>


class TexturesTestDelegate : public vkme::DrawLoopDelegate, public vkme::UserInterfaceDelegate {
public:
    void init(vkme::VulkanData * vulkanData);
    void initFrameResources(vkme::core::DescriptorSetAllocator * allocator);
    void swapchainResized(VkExtent2D newExtent);
    VkImageLayout draw(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const vkme::core::Image* colorImage,
        const vkme::core::Image* depthImage,
        vkme::core::FrameResources& frameResources
    );
    void drawUI();
    
    void cleanup();

protected:
    vkme::VulkanData * _vulkanData;
    
    std::shared_ptr<vkme::core::Image> _drawImage;
    
    VkPipelineLayout _pipelineLayout;
    VkPipeline _pipeline;
    VkPipeline _transparentPipeline;
    bool _transparentMaterial = false;
        
    std::vector<std::shared_ptr<vkme::geo::Model>> _models;
    
    struct SceneData
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewProj;
        glm::vec4 ambientColor;
        glm::vec4 sunlightDirection;
        glm::vec4 sunlightColor;
    };
    
    SceneData _sceneData;
    VkDescriptorSetLayout _sceneDataDescriptorLayout;
    
    std::unique_ptr<vkme::core::Image> _textureImage;
    VkDescriptorSetLayout _imageDescriptorLayout;
    VkSampler _imageSampler;
    
    // 0: no rotation, 1: x, 2: y, 3: z
    uint32_t _rotateAxis = 0;
    
    void initPipeline();
    void initScene();
    void initMesh();

    void drawBackground(VkCommandBuffer cmd, uint32_t currentFrame);
    void drawGeometry(
        VkCommandBuffer cmd,
        VkImageView currentImage,
        VkExtent2D imageExtent,
        const vkme::core::Image* depthImage,
        uint32_t currentFrame,
        vkme::core::FrameResources& frameResources
    );
};
