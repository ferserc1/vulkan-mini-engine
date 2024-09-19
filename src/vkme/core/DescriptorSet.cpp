#include <vkme/core/DescriptorSet.hpp>

namespace vkme {
namespace core {

void DescriptorSet::init(
    vkme::VulkanData * vd,
    VkDescriptorSet ds
) {
    _vulkanData = vd;
    _ds = ds;
}

void DescriptorSet::updateImage(
    uint32_t binding,
    VkDescriptorType type,
    VkImageView imageView,
    VkImageLayout layout
) {
    _imageInfo.imageLayout = layout;
    _imageInfo.imageView = imageView;
    
    VkWriteDescriptorSet writeInfo = {};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstBinding = binding;
    writeInfo.dstSet = _ds;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = type;
    writeInfo.pImageInfo = &_imageInfo;
    writeInfo.pBufferInfo = nullptr;
    writeInfo.pTexelBufferView = nullptr;
    vkUpdateDescriptorSets(_vulkanData->device(), 1, &writeInfo, 0, nullptr);
}

}
}
