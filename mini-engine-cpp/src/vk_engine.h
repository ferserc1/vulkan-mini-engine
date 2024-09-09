// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

#include <vk_mesh.h>

constexpr unsigned int FRAME_OVERLAP = 2;

struct DeletionQueue
{
    std::deque<std::function<void()>> deleters;
    
    void push_function(std::function<void()>&& function)
    {
        deleters.push_back(function);
    }
    
    void flush()
    {
        for (auto it = deleters.rbegin(); it != deleters.rend(); ++it) {
            (*it)();
        }
        
        deleters.clear();
    }
};

class VulkanEngine {
public:

	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	static VulkanEngine& Get();

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();
    
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice _chosenGPU;
    VkDevice _device;
    VkSurfaceKHR _surface;
    
    bool bUseValidationLayers = true;
    
    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImageFormat;
    
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    VkExtent2D _swapchainExtent;
    
    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;
    
    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;
    
    VkRenderPass _renderPass;
    
    std::vector<VkFramebuffer> _framebuffers;
    
    VkSemaphore _presentSemaphore, _renderSemaphore;
    VkFence _renderFence;
    
    VkPipelineLayout _trianglePipelineLayout;
    VkPipeline _trianglePipeline;
    VkPipeline _redTrianglePipeline;
    
    int _selectedShader = 0;
    
    DeletionQueue _mainDeletionQueue;
    
    VmaAllocator _allocator;
    
    VkPipeline _meshPipeline;
    Mesh _triangleMesh;
    
private:
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_sync_structures();
    void init_default_renderpass();
    void init_framebuffers();
    void init_pipelines();
    
    void load_meshes();
    
    void upload_mesh(Mesh& mesh);
    
    void create_swapchain(uint32_t width, uint32_t height);
    
    bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);
};

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    
    VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
};

