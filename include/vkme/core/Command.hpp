
#pragma once

#include <vkme/core/common.hpp>

namespace vkme {

class VulkanData;

namespace core {

class Command {
public:
    void init(VulkanData* vulkanData, vkb::Device *bDevice);
    
    VkCommandPool createCommandPool(VkCommandPoolCreateFlags flags);
    
    VkCommandBuffer allocateCommandBuffer(VkCommandPool pool, uint32_t count = 1);
    
    void destroyComandPool(VkCommandPool pool);
    
    VkDevice device() const;
    inline VkQueue graphicsQueue() const { return _graphicsQueue; }
    inline uint32_t graphicsQueueFamily() const { return _graphicsQueueFamily; }

    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

protected:
    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;

    VkCommandPool _immediateCmdPool;
    VkCommandBuffer _immediateCmdBuffer;
    VkFence _immediateCmdFence;

private:
    VulkanData* _vulkanData = nullptr;
};

}
}
