#pragma once

#include <vkme/core/common.hpp>
#include <vkme/VulkanData.hpp>
#include <vkme/core/Image.hpp>
#include <vkme/core/Buffer.hpp>
#include <vector>

namespace vkme {
namespace core {

class Buffer;

class DescriptorSet {
public:

    void init(vkme::VulkanData * vd, VkDescriptorSet ds);
    
    // To update a descriptor set:
    // - call beginUpdate()
    // - call addImage and/or addBuffer until you
    //   complete the descriptor set info
    // - call endUpdate()
    
    // OR
    
    // If the descriptor set only have one image
    // or one buffer, you can use the funtions
    // updateImage() and updateBuffer(), that
    // does the previous steps for you in one call
    
    inline void updateImage(
        uint32_t binding,
        VkDescriptorType type,
        VkImageView imageView,
        VkImageLayout layout,
        VkSampler sampler = VK_NULL_HANDLE
    ) {
        beginUpdate();
        addImage(
            binding,
            type,
            imageView,
            layout,
            sampler
        );
        endUpdate();
    }
    
    void updateBuffer(
        uint32_t binding,
        VkDescriptorType type,
        Buffer* buffer,
        size_t size,
        size_t offset
    );
    
    inline void beginUpdate() { clear(); }
    
    inline void addImage(
        uint32_t binding,
        VkDescriptorType type,
        Image* image,
        VkImageLayout layout,
        VkSampler sampler = VK_NULL_HANDLE
    ) {
        addImage(binding, type, image->imageView(), layout, sampler);
    }
    
    void addImage(
        uint32_t binding,
        VkDescriptorType type,
        VkImageView imageView,
        VkImageLayout layout,
        VkSampler sampler = VK_NULL_HANDLE
    );
    
    void addBuffer(
        uint32_t binding,
        VkDescriptorType type,
        Buffer* buffer,
        size_t size,
        size_t offset
    );
    
    void addBuffer(
        uint32_t binding,
        VkDescriptorType type,
        VkBuffer buffer,
        size_t size,
        size_t offset
    );
    
    void endUpdate();
    
    // Clear all descriptor writes to add images and buffers again
    void clear();

    inline VkDescriptorSet descriptorSet() const { return _ds; }

    inline VkDescriptorSet* operator&() { return &_ds; }

protected:
    VulkanData * _vulkanData;
    
    std::deque<VkDescriptorImageInfo> _imageInfos;
    std::deque<VkDescriptorBufferInfo> _bufferInfos;
    std::vector<VkWriteDescriptorSet> _writes;
    
    //VkDescriptorImageInfo _imageInfo;
    //VkDescriptorBufferInfo _bufferInfo;
    VkDescriptorSet _ds;
};
}
}
