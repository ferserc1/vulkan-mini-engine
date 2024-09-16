
#include <mini_engine/DrawLoop.hpp>
#include <mini_engine/core/common.hpp>
#include <mini_engine/core/FrameResources.hpp>
#include <mini_engine/core/Init.hpp>
#include <mini_engine/core/Image.hpp>

namespace miniengine {


void DrawLoop::draw()
{
    if (!_vulkanData) {
        throw new std::runtime_error("DrawLoop::draw(): the frame cannot be rendered because the VulkanData object has not been set.");
    }
    
    // Vulkan objects
    VkDevice dev = _vulkanData->device();
    auto swapchainData = _vulkanData->swapchain();
    auto swapchain = swapchainData.swapchain();
    auto graphicsQueue = _vulkanData->command().graphicsQueue();
    
    // Current frame resources
    core::FrameResources frameRes = _vulkanData->currentFrameResources();
    auto cmd = frameRes.commandBuffer;
    auto frameFence = frameRes.frameFence;
    auto swapchainSemaphore = frameRes.swapchainSemaphore;
    auto renderSemaphore = frameRes.renderSemaphore;

    VK_ASSERT(vkWaitForFences(dev, 1, &frameFence, true, 10000000000));
    VK_ASSERT(vkResetFences(dev, 1, &frameFence));
    
    uint32_t swapchainImageIndex;
    VK_ASSERT(vkAcquireNextImageKHR(dev, swapchain, 10000000000, swapchainSemaphore, nullptr, &swapchainImageIndex));
    
    auto swapchainImage = swapchainData.image(swapchainImageIndex);
    
    auto cmdBeginInfo = core::Init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    
    VK_ASSERT(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    
    // Set the swapchain image to a write-ready state
    core::Image::transitionImage(
        cmd,
        swapchainImage,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL
    );
    
    // Clear image
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(_vulkanData->currentFrame() / 120.0f));
    clearValue = { { 0.0f, 0.0f, flash, 1.0f } };
    auto clearRange = core::Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(
        cmd,
        swapchainImage,
        VK_IMAGE_LAYOUT_GENERAL,
        &clearValue, 1, &clearRange
    );
    
    // Transition swapchain image to be presented in the surface
    core::Image::transitionImage(
        cmd,
        swapchainImage,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );
    
    // End command buffer
    VK_ASSERT(vkEndCommandBuffer(cmd));
    
    // Submit command buffer
    auto cmdInfo = core::Init::commandBufferSubmitInfo(cmd);
    auto waitSemaphoreInfo = core::Init::semaphoreSubmitInfo(
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        swapchainSemaphore
    );
    auto signalSemaphoreInfo = core::Init::semaphoreSubmitInfo(
        VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        renderSemaphore
    );
    auto submitInfo = core::Init::submitInfo(&cmdInfo, &signalSemaphoreInfo, &waitSemaphoreInfo);
    VK_ASSERT(vkQueueSubmit2KHR(graphicsQueue, 1, &submitInfo, frameFence));
    
    // Present frame
    auto presentInfo = core::Init::presentInfo(swapchain, renderSemaphore, swapchainImageIndex);
    VK_ASSERT(vkQueuePresentKHR(graphicsQueue, &presentInfo));
    
    // Next frame
    _vulkanData->nextFrame();
}

}
