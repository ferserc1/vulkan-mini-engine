// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

constexpr unsigned int FRAME_OVERLAP = 2;

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
    
private:
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_sync_structures();
    void init_default_renderpass();
    void init_framebuffers();
    
    void create_swapchain(uint32_t width, uint32_t height);
};
