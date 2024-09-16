#pragma once

#include <vulkan/vulkan.h>

#include <mini_engine/core/Command.hpp>

namespace miniengine {
namespace core {

struct FrameResources {
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore swapchainSemaphore;
    VkSemaphore renderSemaphore;
    VkFence frameFence;
    
    void init(Command * command);
    void cleanup();
    
private:
    Command * _command;
};

constexpr unsigned int FRAME_OVERLAP = 2;

}
}
