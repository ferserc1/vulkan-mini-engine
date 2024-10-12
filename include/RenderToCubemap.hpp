#pragma once

#include <vkme/core/Image.hpp>
#include <vkme/VulkanData.hpp>
#include <vkme/DrawLoop.hpp>
#include <vkme/UserInterface.hpp>
#include <vkme/geo/mesh_data.hpp>
#include <vkme/geo/Model.hpp>

struct CubeMapRenderer {

    std::shared_ptr<vkme::geo::Model> sphere;

    struct SkySpherePushConstant {
        VkDeviceAddress vertexBufferAddress;
        int currentFace;
    };

    struct ProjectionData {
		glm::mat4 view[6];
		glm::mat4 proj;
    };

	VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::unique_ptr<vkme::core::Buffer> projectionDataBuffer;
	std::unique_ptr<vkme::core::DescriptorSet> projectionDataDescriptorSet;
    VkDescriptorSetLayout projectionDataDescriptorSetLayout;
    ProjectionData projectionData;

	std::shared_ptr<vkme::core::Image> skyImage;
	VkDescriptorSetLayout skyImageDescriptorSetLayout;
    std::unique_ptr<vkme::core::DescriptorSet> skyImageDescriptorSet;
	VkSampler skyImageSampler;

	void initImages(vkme::VulkanData*);
	void initPipeline(vkme::VulkanData*);
	void initScene(vkme::VulkanData*, vkme::core::DescriptorSetAllocator* dsAllocator);
    void draw(VkCommandBuffer cmd, uint32_t currentFrame);
    
    // This image contains the cube map image, with 6 layers, and the cube map image view
	std::shared_ptr<vkme::core::Image> cubeMapImage;
	// In this array we store the image views for each face of the cube map, needed to render
    // to the cube map image
    std::vector<VkImageView> cubeMapImageViews;
};

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
    
    std::unique_ptr<vkme::core::Buffer> sceneDataBuffer;
    std::unique_ptr<vkme::core::DescriptorSet> sceneDataDescriptorSet;
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
    //std::shared_ptr<vkme::core::Image> _rttImage;
    //std::vector<VkImageView> _rttImageViews;
	//std::shared_ptr<vkme::core::Image> _rttDepthImage;
    
	CubeMapRenderer _cubeMapRenderer;
    SceneCubemap _scene;
        
    // Descriptor set allocator for materials
    std::unique_ptr<vkme::core::DescriptorSetAllocator> _descriptorSetAllocator;
    
    // 0: no rotation, 1: x, 2: y, 3: z
    uint32_t _rotateAxis = 0;
    uint32_t _rotateAxisCube = 0;
    
    void initScenes();
    void initMeshScene(SceneCubemap&);
    //void initMeshScene2(SceneCubemap&);

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
