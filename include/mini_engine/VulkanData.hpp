#pragma once

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <VkBootstrap.h>

#include <mini_engine/Swapchain.hpp>

namespace miniengine {

class VulkanData {
public:

    void init(SDL_Window * window);

    void cleanup();

    inline VkPhysicalDevice physicalDevice() const { return _physicalDevice; }
    inline VkDevice device() const { return _device; }
    inline VkSurfaceKHR surface() const { return _surface;  }
    inline Swapchain& swapchain() { return _swapchain; }
    inline const Swapchain& swapchain() const { return _swapchain; }

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

    Swapchain _swapchain;

    void createInstance();
    void createSurface();
    void createDevicesAndQueues();
};

}
