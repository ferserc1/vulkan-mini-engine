
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

    auto cmdPoolInfo = Info::commandPoolCreateInfo(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_ASSERT(vkCreateCommandPool(_vulkanData->device(), &cmdPoolInfo, nullptr, &_immediateCmdPool));
    
    auto cmdAllocInfo = Info::commandBufferAllocateInfo(_immediateCmdPool);
    VK_ASSERT(vkAllocateCommandBuffers(_vulkanData->device(), &cmdAllocInfo, &_immediateCmdBuffer));
    
    auto fenceInfo = Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_ASSERT(vkCreateFence(_vulkanData->device(), &fenceInfo, nullptr, &_immediateCmdFence));
    _vulkanData->cleanupManager().push([&](VkDevice) {
        vkDestroyCommandPool(_vulkanData->device(), _immediateCmdPool, nullptr);
        vkDestroyFence(_vulkanData->device(), _immediateCmdFence, nullptr);
    });
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

void Command::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    VK_ASSERT(vkResetFences(_vulkanData->device(), 1, &_immediateCmdFence));
	VK_ASSERT(vkResetCommandBuffer(_immediateCmdBuffer, 0));

	VkCommandBufferBeginInfo cmdBeginInfo = Info::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_ASSERT(vkBeginCommandBuffer(_immediateCmdBuffer, &cmdBeginInfo));

	function(_immediateCmdBuffer);

	VK_ASSERT(vkEndCommandBuffer(_immediateCmdBuffer));

	auto cmdInfo = Info::commandBufferSubmitInfo(_immediateCmdBuffer);
    auto submit = Info::submitInfo(&cmdInfo, nullptr, nullptr);

    vkme::core::queueSubmit2(_graphicsQueue, 1, &submit, _immediateCmdFence);

	VK_ASSERT(vkWaitForFences(_vulkanData->device(), 1, &_immediateCmdFence, true, 9999999999));
}

}
}
