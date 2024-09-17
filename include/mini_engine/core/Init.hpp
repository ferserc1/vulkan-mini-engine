
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
    static VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
    static VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
    static VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd);
    static VkSubmitInfo2 submitInfo(VkCommandBufferSubmitInfo* cmdInfo, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);
    static VkSubmitInfo2 submitInfo(
        VkCommandBuffer cmd,
        VkPipelineStageFlags2 waitSemaphoreStageFlags, VkSemaphore waitSemaphore,
        VkPipelineStageFlags2 signalSemaphoreStageFlags, VkSemaphore signalSemaphore
    );
    static VkPresentInfoKHR presentInfo(VkSwapchainKHR& swapchain, VkSemaphore& waitSemaphore, uint32_t& imageIndex);
    static VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
    static VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
};

}
}
