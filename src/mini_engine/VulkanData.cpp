#include <mini_engine/VulkanData.hpp>

#include <VkBootstrap.h>

namespace miniengine {

void VulkanData::init(SDL_Window * window)
{
    _window = window;

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window, &width, &height);

    createInstance();
    createSurface();
    createDevicesAndQueues();
    _swapchain.init(this, uint32_t(width), uint32_t(height));
    createFrameResources();
}

void VulkanData::cleanup()
{
    vkDeviceWaitIdle(_device);
    
    cleanupFrameResources();
    
    _swapchain.cleanup();

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);

    vkb::destroy_debug_utils_messenger(_instance, _debugMessenger, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

void  VulkanData::createInstance()
{
    vkb::InstanceBuilder builder;

    auto instanceBuilder = builder.set_app_name("Vulkan MiniEngine")
        .request_validation_layers(_debugLayers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();

    _vkbInstance = instanceBuilder.value();
    _instance = _vkbInstance.instance;
    _debugMessenger = _vkbInstance.debug_messenger;
}

void  VulkanData::createSurface()
{
    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

}

void  VulkanData::createDevicesAndQueues()
{
    VkPhysicalDeviceVulkan13Features features13 = {};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12 = {};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector { _vkbInstance };
    vkb::PhysicalDevice physicalDevice = selector
        .set_minimum_version(1, 2)
        .set_required_features_13(features13)
        .set_required_features_12(features12)
        .set_surface(_surface)
        .select()
        .value();

    vkb::DeviceBuilder deviceBuilder{ physicalDevice };

    vkb::Device vkbDevice = deviceBuilder.build().value();

    _physicalDevice = physicalDevice.physical_device;
    _device = vkbDevice.device;
    
    _command.init(this, &vkbDevice);
}

void VulkanData::createFrameResources()
{
    for (int i = 0; i < core::FRAME_OVERLAP; ++i)
    {
        _frameResources[i].init(&_command);
    }
}

void VulkanData::cleanupFrameResources()
{
    for (int i = 0; i < core::FRAME_OVERLAP; ++i)
    {
        _frameResources[i].cleanup();
    }
}

}
