#pragma once

#include <vkme/core/Image.hpp>
#include <vkme/VulkanData.hpp>
#include <vkme/DrawLoop.hpp>
#include <vkme/UserInterface.hpp>
#include <vkme/geo/mesh_data.hpp>
#include <vkme/geo/Model.hpp>
#include <vkme/tools/SphereToCubemapRenderer.hpp>
#include <vkme/tools/CubemapRenderer.hpp>
#include <vkme/tools/SkyboxRenderer.hpp>

struct SceneDataCubemap
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor;
};

struct SceneCubemap {

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<std::shared_ptr<vkme::geo::Model>> models;
    
    SceneDataCubemap sceneData;
    VkDescriptorSetLayout sceneDataDescriptorLayout;
    
    std::shared_ptr<vkme::core::Image> textureImage;
    VkDescriptorSetLayout imageDescriptorLayout;
    VkSampler imageSampler;
    
    void initPipeline(vkme::VulkanData*);
    void initScene(vkme::VulkanData*, vkme::core::DescriptorSetAllocator * dsAllocator, const glm::mat4& proj);
};

class RenderToCubemap : public vkme::DrawLoopDelegate, public vkme::UserInterfaceDelegate {
public:
    void init(vkme::VulkanData * vulkanData);
    void initFrameResources(vkme::core::DescriptorSetAllocator * allocator);
    void swapchainResized(VkExtent2D newExtent);
	void update(int32_t currentFrame, vkme::core::FrameResources& frameResources);

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

    // Convert the equirectangular texture into cube map
    std::unique_ptr<vkme::tools::SphereToCubemapRenderer> _sphereToCubeRenderer;
    
    // Convert reflection cube map into another cube map, in this case, a tinted color cubemap
    std::unique_ptr<vkme::tools::CubemapRenderer> _cubeMapRenderer;
    // Descriptor set to pass the tint color to the cubemap renderer shader
    std::unique_ptr<vkme::core::DescriptorSet> _tintColorDS;
    struct TintColorData {
        glm::vec4 tintColor;
    };
    std::unique_ptr<vkme::core::Buffer> _tintColorBuffer;
    
    SceneCubemap _scene;
        
    // Descriptor set allocator for materials
    std::unique_ptr<vkme::core::DescriptorSetAllocator> _descriptorSetAllocator;
    
    // 0: no rotation, 1: x, 2: y, 3: z
    uint32_t _rotateAxis = 0;
    uint32_t _rotateAxisCube = 0;
    float _cameraX = 0.0f;
    float _cameraY = 0.0f;
    float _cameraZ = 3.0f;
    float _cameraRotX = 0.0f;
    float _cameraRotY = 0.0f;
    
    // Resources to draw the sky cubemap
    std::shared_ptr<vkme::tools::SkyboxRenderer> _skyboxRenderer;
    
    void initSkyResources();
    
    void initMeshScene(SceneCubemap&);
   
    void drawBackground(VkCommandBuffer cmd, uint32_t currentFrame, const vkme::core::Image* depthImage);
    void drawGeometry(
        VkCommandBuffer cmd,
        VkImageView currentImage,
        VkExtent2D imageExtent,
        const vkme::core::Image* depthImage,
        uint32_t currentFrame,
        vkme::core::FrameResources& frameResources,
        SceneCubemap& scene,
        uint32_t layerCount = 1
    );
};
