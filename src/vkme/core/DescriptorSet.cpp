#include <vkme/core/DescriptorSet.hpp>
#include <vulkan/vulkan.h>

namespace vkme {
namespace core {

void DescriptorSet::init(
    vkme::VulkanData * vd,
    VkDescriptorSet ds
) {
    _vulkanData = vd;
    _ds = ds;
}

void DescriptorSet::addImage(
    uint32_t binding,
    VkDescriptorType type,
    VkImageView imageView,
    VkImageLayout layout,
    VkSampler sampler
) {
    
    VkDescriptorImageInfo imgInfo = {};
    imgInfo.imageView = imageView;
    imgInfo.imageLayout = layout;
    imgInfo.sampler = sampler;
    VkDescriptorImageInfo& info = _imageInfos.emplace_back(imgInfo);
    
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.dstSet = _ds;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;
    
    _writes.push_back(write);
}


void DescriptorSet::addBuffer(
    uint32_t binding,
    VkDescriptorType type,
    VkBuffer buffer,
    size_t size,
    size_t offset
) {
    // TODO: Implement this
    VkDescriptorBufferInfo bufInfo = {};
    bufInfo.buffer = buffer;
    bufInfo.offset = offset;
    bufInfo.range = size;
    VkDescriptorBufferInfo& info = _bufferInfos.emplace_back(bufInfo);
    
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.dstSet = _ds;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;
    _writes.push_back(write);
}

void DescriptorSet::endUpdate()
{
    vkUpdateDescriptorSets(_vulkanData->device(), uint32_t(_writes.size()), _writes.data(), 0, nullptr);
}

void DescriptorSet::clear()
{
    _imageInfos.clear();
    _bufferInfos.clear();
    _writes.clear();
}

}
}
