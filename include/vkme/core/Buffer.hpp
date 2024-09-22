#pragma once

#include <vkme/core/common.hpp>
#include <vkme/VulkanData.hpp>

namespace vkme {
namespace core {

class Buffer {
public:

    static Buffer* createAllocatedBuffer(
        VulkanData * vulkanData,
        size_t allocSize,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memoryUsage
    );

    void cleanup();

    VkDeviceAddress deviceAddress() const;
    
    void* allocatedData();

    inline VkBuffer buffer() const { return _buffer; }
    inline VmaAllocation allocation() const { return _allocation; }
    inline VmaAllocationInfo allocationInfo() const { return _info; }


protected:
    Buffer() = default;

    VulkanData * _vulkanData;

    VkBuffer _buffer = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
    VmaAllocationInfo _info = {};
};

}
}
