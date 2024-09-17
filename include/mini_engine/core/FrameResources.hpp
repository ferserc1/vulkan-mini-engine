#pragma once

#include <vulkan/vulkan.h>

#include <mini_engine/core/Command.hpp>
#include <mini_engine/core/CleanupManager.hpp>

namespace miniengine {
namespace core {

struct FrameResources {
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore swapchainSemaphore;
    VkSemaphore renderSemaphore;
    VkFence frameFence;
    CleanupManager cleanupManager;
    
    void init(Command * command);
    
    // Remove temporary resources used by this frame
    void flushFrameData();
    
    // Remove all the resources used by the frame
    void cleanup();
    
private:
    Command * _command;
};

constexpr unsigned int FRAME_OVERLAP = 2;

}
}
