
#include <vkme/core/extensions.hpp>
#include <vkme/PlatformTools.hpp>

namespace vkme {
namespace core {

/*
    Since the vast majority of Windows devices support Vulkan API 1.3, but MoltenVK only implements
    version 1.2 with most extensions (September 2024), the strategy we use is to use API 1.3 on
    Windows and 1.2 with extensions on macOS.
*/

vkb::InstanceBuilder createInstanceBuilder(const char* appName)
{
    vkb::InstanceBuilder builder;
#ifdef MINI_ENGINE_IS_WINDOWS
    auto instanceBuilder = builder.set_app_name(appName)
        .require_api_version(1, 3, 0);
#else
    auto instanceBuilder = builder.set_app_name(appName)
        .require_api_version(1, 2, 0);
#endif

    return instanceBuilder;
}

// VK_KHR_dynamic_rendering
void cmdBeginRendering(
    VkCommandBuffer                         commandBuffer,
    const VkRenderingInfo*                  pRenderingInfo
) {
#ifdef MINI_ENGINE_IS_WINDOWS
    vkCmdBeginRendering(commandBuffer, pRenderingInfo);
#else
    vkCmdBeginRenderingKHR(commandBuffer, pRenderingInfo);
#endif
}

void cmdEndRendering(
    VkCommandBuffer commandBuffer
) {
#ifdef MINI_ENGINE_IS_WINDOWS
    vkCmdEndRendering(commandBuffer);
#else
    vkCmdEndRenderingKHR(commandBuffer);
#endif
}

// VK_KHR_swapchain
VkResult acquireNextImage(
    VkDevice                                    device,
    VkSwapchainKHR                              swapchain,
    uint64_t                                    timeout,
    VkSemaphore                                 semaphore,
    VkFence                                     fence,
    uint32_t*                                   pImageIndex
) {
    return vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VkResult queuePresent(
    VkQueue                                     queue,
    const VkPresentInfoKHR*                     pPresentInfo
) {
    return vkQueuePresentKHR(queue, pPresentInfo);
}

void destroySurface(
    VkInstance                                  instance,
    VkSurfaceKHR                                surface,
    const VkAllocationCallbacks*                pAllocator
) {
    vkDestroySurfaceKHR(instance, surface, pAllocator);
}

void destroySwapchain(
    VkDevice                                    device,
    VkSwapchainKHR                              swapchain,
    const VkAllocationCallbacks*                pAllocator
) {
    vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

// VK_KHR_synchronization2
VkResult queueSubmit2(
    VkQueue                                     queue,
    uint32_t                                    submitCount,
    const VkSubmitInfo2*                        pSubmits,
    VkFence                                     fence
) {
#ifdef MINI_ENGINE_IS_WINDOWS
    return vkQueueSubmit2(queue, submitCount, pSubmits, fence);
#else
    return vkQueueSubmit2KHR(queue, submitCount, pSubmits, fence);
#endif
}

void cmdPipelineBarrier2(
    VkCommandBuffer                             commandBuffer,
    const VkDependencyInfo*                     pDependencyInfo
) {
#ifdef MINI_ENGINE_IS_WINDOWS
    vkCmdPipelineBarrier2(commandBuffer, pDependencyInfo);
#else
    vkCmdPipelineBarrier2KHR(commandBuffer, pDependencyInfo);
#endif
}

// VK_KHR_copy_commands2
void cmdBlitImage2(
    VkCommandBuffer                             commandBuffer,
    const VkBlitImageInfo2*                     pBlitImageInfo
) {
#ifdef MINI_ENGINE_IS_WINDOWS
	vkCmdBlitImage2(commandBuffer, pBlitImageInfo);
#else
    vkCmdBlitImage2KHR(commandBuffer, pBlitImageInfo);
#endif
}


}
}
