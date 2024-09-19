#pragma once

#include <vkme/core/common.hpp>
#include <vkme/VulkanData.hpp>
#include <vector>

namespace vkme {
namespace core {

class DescriptorSet {
public:

    void init(vkme::VulkanData * vd, VkDescriptorSet ds);
    
    void updateImage(
        uint32_t binding,
        VkDescriptorType type,
        VkImageView imageView,
        VkImageLayout layout
    );

    inline VkDescriptorSet descriptorSet() const { return _ds; }

    inline VkDescriptorSet* operator&() { return &_ds; }

protected:
    VulkanData * _vulkanData;
    VkDescriptorImageInfo _imageInfo;
    VkDescriptorSet _ds;
};
}
}
