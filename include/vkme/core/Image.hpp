
#pragma once

#include <vkme/core/common.hpp>

namespace vkme {

class VulkanData;

namespace core {

class Image {
public:
    /*
     *   Add an image transition command to the command buffer to switch from one layout
     *   to another.
     *
     *   The default configuration of stageMask and accessMask will always work, but will
     *   leave the queue stopped until the the queue stopped until the transition is complete.
     *
     *   It is recommended to fine-tune these parameters for the intended use of the image.
     */
    static void cmdTransitionImage(
        VkCommandBuffer       cmd,
        VkImage               image,
        VkImageLayout         oldLayout,
        VkImageLayout         newLayout,
        VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VkAccessFlags2        srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VkAccessFlags2        dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
        VkImageAspectFlags    aspectMask = 0
    );

    /*
     *  Returns a default initialised VkImageSubresourceRange structure for all mipmaps and layers.
     */
    static VkImageSubresourceRange subresourceRange(VkImageAspectFlags aspectMask);
    
    /*
     *   Setup an image to image copy command
     */
    static void cmdCopy(
        VkCommandBuffer cmd,
        VkImage srcImage,
        VkExtent2D srcSize,
        VkImage dstImage,
        VkExtent2D dstSize
    );
    
    static Image* createAllocatedImage(
        VulkanData * vulkanData,
        VkFormat format,
        VkExtent2D extent,
        VkImageUsageFlags usage,
        VkImageAspectFlags aspectFlags
    );
    
    void cleanup();

    inline VkImage image() const { return _image; }
    inline VkImageView imageView() const { return _imageView; }
    inline VmaAllocation allocation() const { return _allocation; }
    inline const VkExtent3D& extent() const { return _extent; }
    inline const VkExtent2D extent2D() const { return VkExtent2D{ _extent.width, _extent.height }; }
    inline VkFormat format() const { return _format; }

protected:
    VkImage _image = VK_NULL_HANDLE;
    VkImageView _imageView = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
    VkExtent3D _extent = { 0, 0 };
    VkFormat _format;
    
    VulkanData * _vulkanData;
};

}
}