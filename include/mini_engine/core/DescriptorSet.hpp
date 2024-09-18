#pragma once

#include <mini_engine/core/common.hpp>
#include <mini_engine/VulkanData.hpp>
#include <vector>

namespace miniengine {
namespace core {

class DescriptorSet {
public:

    void init(miniengine::VulkanData * vd, VkDescriptorSet ds);
    
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
