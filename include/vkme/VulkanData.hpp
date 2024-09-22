#pragma once

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include <VkBootstrap.h>

#include <vkme/core/Swapchain.hpp>
#include <vkme/core/Command.hpp>
#include <vkme/core/FrameResources.hpp>
#include <vkme/core/CleanupManager.hpp>

namespace vkme {

class VulkanData {
public:

    void init(SDL_Window * window);

    void cleanup();

    inline const SDL_Window* window() const { return _window; }
    inline SDL_Window* window() { return _window; }

    inline VkInstance instance() const { return _instance; }
    inline VkPhysicalDevice physicalDevice() const { return _physicalDevice; }
    inline VkDevice device() const { return _device; }
    inline VkSurfaceKHR surface() const { return _surface;  }
    inline core::Swapchain& swapchain() { return _swapchain; }
    inline const core::Swapchain& swapchain() const { return _swapchain; }
    inline core::Command& command() { return _command; }
    inline const core::Command& command() const { return _command; }

    inline core::FrameResources& currentFrameResources() { return _frameResources[_currentFrame % core::FRAME_OVERLAP]; }
    inline const core::FrameResources& currentFrameResources() const { return _frameResources[_currentFrame % core::FRAME_OVERLAP]; }
    inline uint32_t currentFrame() const { return _currentFrame; }
    inline void nextFrame() { ++_currentFrame; }
    
    inline core::CleanupManager& cleanupManager() { return _cleanupManager; }
    
    inline VmaAllocator allocator() const { return _allocator; }
    
    inline void updateSwapchainSize() { _resizeRequested = true; }
    
    // This function returns true if the swapchain have been resized
    bool newFrame();

protected:
    SDL_Window * _window;

private:

    bool _debugLayers = true;

    vkb::Instance _vkbInstance;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VkSurfaceKHR _surface;

    core::Swapchain _swapchain;
    core::Command _command;
    
    core::FrameResources _frameResources[core::FRAME_OVERLAP];
    uint32_t _currentFrame = 0;
    
    core::CleanupManager _cleanupManager;
    
    VmaAllocator _allocator;
    
    bool _resizeRequested = false;


    void createInstance();
    void createSurface();
    void createDevicesAndQueues();
    void createMemoryAllocator();
    void createFrameResources();
    
    void cleanupFrameResources();
};

}
