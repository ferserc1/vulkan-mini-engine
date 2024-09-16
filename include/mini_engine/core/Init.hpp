
#pragma once

#include <mini_engine/core/common.hpp>

namespace miniengine {
namespace core {

class Init {
public:
    static VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    static VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1);
    static VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);
    static VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);
    
};

}
}
