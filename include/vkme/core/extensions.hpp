#pragma once

#include <vkme/PlatformTools.hpp>
#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

/*
    Currently MoltenVK does not support Vulkan API 1.3, so we have to use the KHR extension functions
    from API 1.2 on macOS. On Windows we have the problem that we have to load the KHR functions manually,
    or use API 1.3. A simpler solution to manage these problems is to create a facade for these functions,
    instead of using the Vulkan functions directly.
    
    This file provides that facade to make these calls seamless.
*/

namespace vkme {
namespace core {

// Returns the Instance Builder from vk-bootstram. Inside this function
// we can decide if we want to use the KHR functions or the API 1.3 functions.
vkb::InstanceBuilder createInstanceBuilder(const char* appName);

// VK_KHR_dynamic_rendering
void cmdBeginRendering(
    VkCommandBuffer                         commandBuffer,
    const VkRenderingInfo*                  pRenderingInfo);

void cmdEndRendering
    (VkCommandBuffer commandBuffer);

// VK_KHR_swapchain
VkResult acquireNextImage(
    VkDevice                                    device,
    VkSwapchainKHR                              swapchain,
    uint64_t                                    timeout,
    VkSemaphore                                 semaphore,
    VkFence                                     fence,
    uint32_t*                                   pImageIndex);

VkResult queuePresent(
    VkQueue                                     queue,
    const VkPresentInfoKHR*                     pPresentInfo);

void destroySurface(
    VkInstance                                  instance,
    VkSurfaceKHR                                surface,
    const VkAllocationCallbacks*                pAllocator);

void destroySwapchain(
    VkDevice                                    device,
    VkSwapchainKHR                              swapchain,
    const VkAllocationCallbacks*                pAllocator);

// VK_KHR_synchronization2
VkResult queueSubmit2(
    VkQueue                                     queue,
    uint32_t                                    submitCount,
    const VkSubmitInfo2*                        pSubmits,
    VkFence                                     fence);

void cmdPipelineBarrier2(
    VkCommandBuffer                             commandBuffer,
    const VkDependencyInfo*                     pDependencyInfo);

// VK_KHR_copy_commands2
void cmdBlitImage2(
    VkCommandBuffer                             commandBuffer,
    const VkBlitImageInfo2*                     pBlitImageInfo);

}
}
