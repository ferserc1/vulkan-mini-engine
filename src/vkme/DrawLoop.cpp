
#include <vkme/DrawLoop.hpp>
#include <vkme/core/common.hpp>
#include <vkme/core/FrameResources.hpp>
#include <vkme/core/Info.hpp>
#include <vkme/core/Image.hpp>
#include <vkme/PlatformTools.hpp>
#include <vkme/core/DescriptorSetAllocator.hpp>

namespace vkme {

void DrawLoopDelegate::cmdSetDefaultViewportAndScissor(VkCommandBuffer cmd, VkExtent2D viewportExtent)
{
    VkViewport viewport = {};
    viewport.x = 0; viewport.y = 0;
    viewport.width = float(viewportExtent.width);
    viewport.height = float(viewportExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = viewportExtent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void DrawLoop::init(VulkanData * vulkanData, UserInterface * ui)
{
    _vulkanData = vulkanData;
    _userInterface = ui;

    if (_drawDelegate)
    {
        _drawDelegate->init(vulkanData);
    }
    
    // Initialize frame resources
    _vulkanData->iterateFrameResources([&](core::FrameResources& res) {
        res.descriptorAllocator->init(_vulkanData);
        initFrameResources(res.descriptorAllocator);
    });
}

void DrawLoop::swapchainResized()
{
    if (_drawDelegate)
    {
        auto newExtent = _vulkanData->swapchain().extent();
        _drawDelegate->swapchainResized(newExtent);
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
    core::FrameResources& frameRes = _vulkanData->currentFrameResources();
    auto cmd = frameRes.commandBuffer;
    auto frameFence = frameRes.frameFence;
    auto swapchainSemaphore = frameRes.swapchainSemaphore;
    auto renderSemaphore = frameRes.renderSemaphore;

    VK_ASSERT(vkWaitForFences(dev, 1, &frameFence, true, 10000000000));
    VK_ASSERT(vkResetFences(dev, 1, &frameFence));
    
      frameRes.flushFrameData();
    
    uint32_t swapchainImageIndex;
    auto acquireResult = core::acquireNextImage(dev, swapchain, 10000000000, swapchainSemaphore, nullptr, &swapchainImageIndex);
    if (acquireResult == VK_SUBOPTIMAL_KHR)
    {
        _vulkanData->updateSwapchainSize();
    }
    else if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        _vulkanData->updateSwapchainSize();
        return;
    }

	if (_drawDelegate.get())
	{
		_drawDelegate->update(_vulkanData->currentFrame(), frameRes);
	}
    
    auto swapchainImage = swapchainData.colorImage(swapchainImageIndex);
    auto depthImage = swapchainData.depthImage();
    
    auto cmdBeginInfo = core::Info::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    
    VK_ASSERT(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    
    core::Image::cmdTransitionImage(
        cmd,
        depthImage->image(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    );
    
    auto lastSwapchainLayout = draw(
        cmd,
        swapchainImage,
        depthImage,
        frameRes
    );
    //auto lastSwapchainLayout = draw(cmd, swapchainImage, swapchainData.extent(), swapchainData.depthImage());
    
    if (lastSwapchainLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        core::Image::cmdTransitionImage(
            cmd,
            swapchainImage->image(),
            lastSwapchainLayout,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
    }
    
    // TODO: Instead of using the swapchain image to render the user interface, we could use another image
    // and combine it with the swap chain here
    _userInterface->draw(
        cmd,
        swapchainImage->imageView()
    );
    
    core::Image::cmdTransitionImage(
        cmd,
        swapchainImage->image(),
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
    VK_ASSERT(core::queueSubmit2(graphicsQueue, 1, &submitInfo, frameFence));

    // Present frame
    auto presentInfo = core::Info::presentInfo(swapchain, renderSemaphore, swapchainImageIndex);
    auto presentResult = core::queuePresent(graphicsQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
    {
        _vulkanData->updateSwapchainSize();
    }
    
    // Next frame
    _vulkanData->nextFrame();
}

}
