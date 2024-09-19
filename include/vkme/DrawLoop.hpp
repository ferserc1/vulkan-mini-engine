#pragma once

#include <vkme/VulkanData.hpp>
#include <vkme/core/Image.hpp>
#include <vkme/UserInterface.hpp>

#include <memory>

namespace vkme {

class DrawLoopDelegate {
public:
    virtual ~DrawLoopDelegate() {}

    virtual void init(VulkanData *) {}

    // This call must return the last swapchainImage layout, or VK_IMAGE_LAYOUT_UNDEFINED if it is not used
    virtual VkImageLayout draw(VkCommandBuffer cmd, VkImage swapchainImage, VkExtent2D imageExtent, uint32_t currentFrame) = 0;
};

class DrawLoop {
public:
    void init(VulkanData * vulkanData, UserInterface * ui);
    
    void acquireAndPresent();
    
    VkImageLayout draw(VkCommandBuffer cmd, VkImage swapchainImage, VkExtent2D imageExtent)
    {
        if (_drawDelegate) {
            return _drawDelegate->draw(cmd, swapchainImage, imageExtent, _vulkanData->currentFrame());
        }
        else {
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }
    
    inline void setDelegate(std::shared_ptr<DrawLoopDelegate> delegate) { _drawDelegate = delegate; }

protected:
    VulkanData * _vulkanData = nullptr;
    UserInterface * _userInterface = nullptr;
    
    std::shared_ptr<DrawLoopDelegate> _drawDelegate;
};

}
