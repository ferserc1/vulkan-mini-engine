#pragma once

#include <vulkan/vulkan.h>

#include <vkme/core/Command.hpp>
#include <vkme/core/CleanupManager.hpp>
//#include <vkme/core/DescriptorSetAllocator.hpp>

namespace vkme {
namespace core {

class DescriptorSetAllocator;

struct FrameResources {
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore swapchainSemaphore;
    VkSemaphore renderSemaphore;
    VkFence frameFence;
    CleanupManager cleanupManager;
    DescriptorSetAllocator* descriptorAllocator;
    
    void init(VkDevice device, Command * command);
    
    // Remove temporary resources used by this frame
    void flushFrameData();
    
    // Remove all the resources used by the frame
    void cleanup();
    
private:
    VkDevice _device;
    Command * _command;
};

constexpr unsigned int FRAME_OVERLAP = 2;

}
}
