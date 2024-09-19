
#include <vkme/DrawLoop.hpp>
#include <vkme/core/common.hpp>
#include <vkme/core/FrameResources.hpp>
#include <vkme/core/Info.hpp>
#include <vkme/core/Image.hpp>
#include <vkme/PlatformTools.hpp>

namespace vkme {

void DrawLoop::init(VulkanData * vulkanData, UserInterface * ui)
{
    _vulkanData = vulkanData;
    _userInterface = ui;

    if (_drawDelegate)
    {
        _drawDelegate->init(vulkanData);
    }
}

void DrawLoop::acquireAndPresent()
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
    
    frameRes.flushFrameData();
    
    uint32_t swapchainImageIndex;
    VK_ASSERT(vkAcquireNextImageKHR(dev, swapchain, 10000000000, swapchainSemaphore, nullptr, &swapchainImageIndex));
    
    auto swapchainImage = swapchainData.image(swapchainImageIndex);
    auto swapchainImageView = swapchainData.imageView(swapchainImageIndex);
    
    auto cmdBeginInfo = core::Info::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    
    VK_ASSERT(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    
    auto lastSwapchainLayout = draw(cmd, swapchainImage, swapchainData.extent());
    
    if (lastSwapchainLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        core::Image::cmdTransitionImage(
            cmd,
            swapchainImage,
            lastSwapchainLayout,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
    }
    
    // TODO: Instead of using the swapchain image to render the user interface, we could use another image
    // and combine it with the swap chain here
    _userInterface->draw(cmd, swapchainImageView);
    
    core::Image::cmdTransitionImage(
        cmd,
        swapchainImage,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );
    
    // End command buffer
    VK_ASSERT(vkEndCommandBuffer(cmd));
    
    // Submit command buffer
    auto cmdInfo = core::Info::commandBufferSubmitInfo(cmd);
    auto waitSemaphoreInfo = core::Info::semaphoreSubmitInfo(
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        swapchainSemaphore
    );
    auto signalSemaphoreInfo = core::Info::semaphoreSubmitInfo(
        VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        renderSemaphore
    );
    auto submitInfo = core::Info::submitInfo(&cmdInfo, &signalSemaphoreInfo, &waitSemaphoreInfo);
#ifdef MINI_ENGINE_IS_WINDOWS
    VK_ASSERT(vkQueueSubmit2(graphicsQueue, 1, &submitInfo, frameFence));
#else
    VK_ASSERT(vkQueueSubmit2KHR(graphicsQueue, 1, &submitInfo, frameFence));
#endif

    // Present frame
    auto presentInfo = core::Info::presentInfo(swapchain, renderSemaphore, swapchainImageIndex);
    VK_ASSERT(vkQueuePresentKHR(graphicsQueue, &presentInfo));
    
    // Next frame
    _vulkanData->nextFrame();
}

}
