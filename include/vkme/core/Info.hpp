
#pragma once

#include <vkme/core/common.hpp>

namespace vkme {
namespace core {

class Info {
public:
    static VkCommandPoolCreateInfo commandPoolCreateInfo(
        uint32_t queueFamilyIndex,
        VkCommandPoolCreateFlags flags = 0
    );

    static VkCommandBufferAllocateInfo commandBufferAllocateInfo(
        VkCommandPool pool,
        uint32_t count = 1
    );

    static VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);

    static VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

    static VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);

    static VkSemaphoreSubmitInfo semaphoreSubmitInfo(
        VkPipelineStageFlags2 stageMask,
        VkSemaphore semaphore
    );

    static VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd);

    static VkSubmitInfo2 submitInfo(
        VkCommandBufferSubmitInfo* cmdInfo,
        VkSemaphoreSubmitInfo* signalSemaphoreInfo,
        VkSemaphoreSubmitInfo*
        waitSemaphoreInfo
    );

    static VkSubmitInfo2 submitInfo(
        VkCommandBuffer cmd,
        VkPipelineStageFlags2 waitSemaphoreStageFlags, VkSemaphore waitSemaphore,
        VkPipelineStageFlags2 signalSemaphoreStageFlags, VkSemaphore signalSemaphore
    );

    static VkPresentInfoKHR presentInfo(
        VkSwapchainKHR& swapchain,
        VkSemaphore& waitSemaphore,
        uint32_t& imageIndex
    );

    static VkImageCreateInfo imageCreateInfo(
        VkFormat format,
        VkImageUsageFlags usageFlags,
        VkExtent3D extent
    );

    static VkImageViewCreateInfo imageViewCreateInfo(
        VkFormat format,
        VkImage image,
        VkImageAspectFlags aspectFlags
    );

    static VkRenderingAttachmentInfo attachmentInfo(
        VkImageView view,
        VkClearValue* clearValue,
        VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    static VkRenderingInfo renderingInfo(
        VkExtent2D renderExtent,
        VkRenderingAttachmentInfo* colorAttachment,
        VkRenderingAttachmentInfo* depthAttachment
    );

    static VkPipelineLayoutCreateInfo pipelineLayoutInfo();
};

}
}
