
#include <vkme/core/Command.hpp>
#include <vkme/core/Info.hpp>

#include <vkme/VulkanData.hpp>


namespace vkme {
namespace core {

void Command::init(VulkanData *vulkanData, vkb::Device *bDevice)
{
    _vulkanData = vulkanData;
    
    _graphicsQueue = bDevice->get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = bDevice->get_queue_index(vkb::QueueType::graphics).value();
}

VkCommandPool Command::createCommandPool(VkCommandPoolCreateFlags flags)
{
    VkCommandPool pool;
    auto poolInfo = Info::commandPoolCreateInfo(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    
    VK_ASSERT(vkCreateCommandPool(_vulkanData->device(), &poolInfo, nullptr, &pool));
    
    return pool;
}
    
VkCommandBuffer Command::allocateCommandBuffer(VkCommandPool pool, uint32_t count)
{
    auto allocInfo = Info::commandBufferAllocateInfo(pool, count);
    VkCommandBuffer buffer;
    
    VK_ASSERT(vkAllocateCommandBuffers(_vulkanData->device(), &allocInfo, &buffer));
    
    return buffer;
}

void Command::destroyComandPool(VkCommandPool pool)
{
    vkDestroyCommandPool(_vulkanData->device(), pool, nullptr);
}

VkDevice Command::device() const
{
    return _vulkanData->device();
}


}
}
