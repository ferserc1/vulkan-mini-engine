
#include <vkme/core/Buffer.hpp>

namespace vkme {
namespace core {

Buffer* Buffer::createAllocatedBuffer(
    VulkanData * vulkanData,
    size_t allocSize,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memoryUsage
) {
    auto buffer = new Buffer();
    
    buffer->_vulkanData = vulkanData;
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = allocSize;
    bufferInfo.usage = usage;
    
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    VK_ASSERT(vmaCreateBuffer(
        vulkanData->allocator(),
        &bufferInfo,
        &allocInfo,
        &buffer->_buffer,
        &buffer->_allocation,
        &buffer->_info
    ));
    
    return buffer;
}

void Buffer::cleanup()
{
    vmaDestroyBuffer(_vulkanData->allocator(), _buffer, _allocation);
    _buffer = VK_NULL_HANDLE;
    _allocation = VK_NULL_HANDLE;
    _info = {};
}

void* Buffer::allocatedData()
{
    return vkme::core::getMappedData(_allocation);
}

VkDeviceAddress Buffer::deviceAddress() const
{
    if (_buffer == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Buffer::deviceAddress() called on a null buffer");
    }

    VkBufferDeviceAddressInfo deviceAddressInfo = {};
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.buffer = _buffer;
    return vkGetBufferDeviceAddress(_vulkanData->device(), &deviceAddressInfo);
}
    
}
}
