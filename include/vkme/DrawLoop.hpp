#pragma once

#include <vkme/VulkanData.hpp>
#include <vkme/core/Image.hpp>
#include <vkme/UserInterface.hpp>
#include <vkme/core/DescriptorSetAllocator.hpp>

#include <memory>

namespace vkme {

class DrawLoopDelegate {
public:
    virtual ~DrawLoopDelegate() {}

    virtual void init(VulkanData *) {}
    
    // This function is called to initialize the frame specific resources,
    // and will be called once per on-flight frame resources. There are
    //  usually two or three frames that are rendered on the fly, so it will
    //  be called two or three times. This function is so we can indicate
    //  the types of descriptor set we're going to use in the rendering, so
    //  we'll use it to initialise the DescriptorSetAllocator
    virtual void initFrameResources(core::DescriptorSetAllocator* dsAllocator) {}
    
    // This function is called when the swapchain has been resized. Inside this function you can
    // recreate any resource that depends on the viewport size, because the VulkanData class
    // ensures that the device is iddle before calling this function.
    //
    // It is mandatory to implement this function to make sure that the delegate will handle the
    // image resizing case, otherwise the rendering may fail. If the delegate does not create any
    // image where it has to render (e.g. if it is going to draw directly in the swapchain) it is
    // not necessary to do anything. If you are going to use other images to render, either you
    // have to re-create them with the new size, or they have to be rendered using the size of
    // that image, and not the swapchain's size.
    virtual void swapchainResized(VkExtent2D newExtent) = 0;

    // Called before create the frame command buffer
    virtual void update(int32_t currentFrame, core::FrameResources&) {}

    // This call must return the last swapchainImage layout, or VK_IMAGE_LAYOUT_UNDEFINED if it is not used
    virtual VkImageLayout draw(VkCommandBuffer cmd, uint32_t currentFrame, const core::Image* colorImage, const core::Image* depthImage, core::FrameResources& frameResources) = 0;

    void cmdSetDefaultViewportAndScissor(VkCommandBuffer cmd, VkExtent2D viewportExtent);
};

class DrawLoop {
public:
    void init(VulkanData * vulkanData, UserInterface * ui);
    
    void acquireAndPresent();
    
    // This function is called when the swapchain has been resized. Inside this function you can
    // recreate any resource that depends on the viewport size, because the VulkanData class
    // ensures that the device is iddle before calling this function.
    void swapchainResized();
    
    void initFrameResources(core::DescriptorSetAllocator* dsAllocator)
    {
        if (_drawDelegate)
        {
            _drawDelegate->initFrameResources(dsAllocator);
        }
    }
    
    VkImageLayout draw(VkCommandBuffer cmd, const core::Image* colorImage, const core::Image* depthImage, core::FrameResources& frameResources)
    {
        if (_drawDelegate) {
            return _drawDelegate->draw(cmd, _vulkanData->currentFrame(), colorImage, depthImage, frameResources);
        }
        else {
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }
    
    inline void setDelegate(std::shared_ptr<DrawLoopDelegate> delegate) { _drawDelegate = delegate; }


protected:
    VulkanData * _vulkanData = nullptr;
    UserInterface * _userInterface = nullptr;
    bool _resizeRequested = false;
    
    std::shared_ptr<DrawLoopDelegate> _drawDelegate;
};

}
