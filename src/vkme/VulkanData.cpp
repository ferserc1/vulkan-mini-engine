#include <vkme/VulkanData.hpp>

#include <VkBootstrap.h>

namespace vkme {

void VulkanData::init(SDL_Window * window)
{
    _window = window;

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window, &width, &height);

    createInstance();
    createSurface();
    createDevicesAndQueues();
    createMemoryAllocator();
    _swapchain.init(this, uint32_t(width), uint32_t(height));
    createFrameResources();
}

void VulkanData::cleanup()
{
    vkDeviceWaitIdle(_device);
    
    _cleanupManager.flush(_device);
    
    cleanupFrameResources();
    
    _swapchain.cleanup();

    vmaDestroyAllocator(_allocator);

    vkDestroyDevice(_device, nullptr);
    
    core::destroySurface(_instance, _surface, nullptr);

    vkb::destroy_debug_utils_messenger(_instance, _debugMessenger, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

bool VulkanData::newFrame()
{
    if (_resizeRequested)
    {
        vkDeviceWaitIdle(_device);

        int w, h;
        SDL_GetWindowSize(_window, &w, &h);
        _swapchain.resize(uint32_t(w), uint32_t(h));
        
        _resizeRequested = false;
        return true;
    }
    
    return false;
}

void VulkanData::createInstance()
{
    auto instanceBuilder = core::createInstanceBuilder("Vulkan MiniEngine")
        .request_validation_layers(_debugLayers)
        .use_default_debug_messenger()
        .build();

    _vkbInstance = instanceBuilder.value();
    _instance = _vkbInstance.instance;
    _debugMessenger = _vkbInstance.debug_messenger;
}

void VulkanData::createSurface()
{
    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);
}

void VulkanData::createDevicesAndQueues()
{
    VkPhysicalDeviceVulkan13Features features13 = {};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12 = {};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    VkPhysicalDeviceVulkan11Features features11 = {};
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	features11.multiview = true;

    vkb::PhysicalDeviceSelector selector { _vkbInstance };
    vkb::PhysicalDevice physicalDevice = selector
        .set_minimum_version(1, 2)
        .set_required_features_13(features13)
        .set_required_features_12(features12)
        .set_required_features_11(features11)
        .set_surface(_surface)
        .select()
        .value();

    vkb::DeviceBuilder deviceBuilder{ physicalDevice };

    vkb::Device vkbDevice = deviceBuilder.build().value();

    _physicalDevice = physicalDevice.physical_device;
    _device = vkbDevice.device;
    
    _command.init(this, &vkbDevice);
}

void VulkanData::createMemoryAllocator()
{
    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.physicalDevice = _physicalDevice;
    allocInfo.device = _device;
    allocInfo.instance = _instance;
    allocInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocInfo, &_allocator);
}

void VulkanData::createFrameResources()
{
    for (int i = 0; i < core::FRAME_OVERLAP; ++i)
    {
        _frameResources[i].init(_device, &_command);
    }
}

void VulkanData::cleanupFrameResources()
{
    for (int i = 0; i < core::FRAME_OVERLAP; ++i)
    {
        _frameResources[i].cleanup();
    }
}

void VulkanData::iterateFrameResources(std::function<void(core::FrameResources&)> cb)
{
    for (auto i = 0; i < core::FRAME_OVERLAP; ++i)
    {
        cb(_frameResources[i]);
    }
}

}
