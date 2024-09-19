#include <vkme/core/FrameResources.hpp>

#include <vkme/core/Info.hpp>

namespace vkme {
namespace core {

void FrameResources::init(Command * command)
{
    _command = command;
    
    // Command pool and command buffer
    commandPool = command->createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffer = command->allocateCommandBuffer(commandPool, 1);
    
    // Synchonization structures
    auto fenceInfo = Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_ASSERT(vkCreateFence(_command->device(), &fenceInfo, nullptr, &frameFence));
    
    auto semaphoreInfo = Info::semaphoreCreateInfo();
    VK_ASSERT(vkCreateSemaphore(_command->device(), &semaphoreInfo, nullptr, &swapchainSemaphore));
    VK_ASSERT(vkCreateSemaphore(_command->device(), &semaphoreInfo, nullptr, &renderSemaphore));
}

void FrameResources::flushFrameData()
{
    cleanupManager.flush();
}

void FrameResources::cleanup()
{
    // Destroy command pool
    _command->destroyComandPool(commandPool);
    
    // Destroy synchronization structures
    vkDestroyFence(_command->device(), frameFence, nullptr);
    vkDestroySemaphore(_command->device(), swapchainSemaphore, nullptr);
    vkDestroySemaphore(_command->device(), renderSemaphore, nullptr);
    
    // Destroy frame cleanup manager
    cleanupManager.flush();
}


}
}
