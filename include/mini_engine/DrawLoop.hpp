#pragma once

#include <mini_engine/VulkanData.hpp>
#include <mini_engine/core/Image.hpp>

#include <memory>

namespace miniengine {

class DrawLoopDelegate {
public:
    virtual void init(VulkanData *) {}
    virtual void draw(VkCommandBuffer cmd, VkImage swapchainImage, VkExtent2D imageExtent, uint32_t currentFrame) = 0;
};

class DrawLoop {
public:
    void init(VulkanData * vulkanData);
    
    void acquireAndPresent();
    
    void draw(VkCommandBuffer cmd, VkImage swapchainImage, VkExtent2D imageExtent)
    {
        if (_drawDelegate) {
            _drawDelegate->draw(cmd, swapchainImage, imageExtent, _vulkanData->currentFrame());
        }
    }
    
    inline void setDrawDelegate(DrawLoopDelegate * delegate) { _drawDelegate = delegate; }

protected:
    VulkanData * _vulkanData = nullptr;
    
    DrawLoopDelegate * _drawDelegate = nullptr;
};

}
