
#include <mini_engine/PlatformTools.hpp>
#include <mini_engine/core/Image.hpp>
#include <mini_engine/core/Init.hpp>

#include <mini_engine/VulkanData.hpp>

namespace miniengine {
namespace core {

void Image::cmdTransitionImage(
    VkCommandBuffer       cmd,
    VkImage               image,
    VkImageLayout         oldLayout,
    VkImageLayout         newLayout,
    VkPipelineStageFlags2 srcStageMask,
    VkAccessFlags2        srcAccessMask,
    VkPipelineStageFlags2 dstStageMask,
    VkAccessFlags2        dstAccessMask,
    VkImageAspectFlags    aspectMask
) {
    VkImageMemoryBarrier2 imageBarrier = {};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.srcStageMask = srcStageMask;
    imageBarrier.srcAccessMask = srcAccessMask;
    imageBarrier.dstStageMask = dstStageMask;
    imageBarrier.dstAccessMask = dstStageMask; 
    imageBarrier.oldLayout = oldLayout;
    imageBarrier.newLayout = newLayout;

    if (aspectMask == 0) {
        aspectMask = newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ?
            VK_IMAGE_ASPECT_DEPTH_BIT :
            VK_IMAGE_ASPECT_COLOR_BIT;
    }
    imageBarrier.subresourceRange = Image::subresourceRange(aspectMask);
    imageBarrier.image = image;

    VkDependencyInfo dependencies = {};
    dependencies.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencies.imageMemoryBarrierCount = 1;
    dependencies.pImageMemoryBarriers = &imageBarrier;
    

#ifdef MINI_ENGINE_IS_WINDOWS
    vkCmdPipelineBarrier2(cmd, &dependencies);
#else
    vkCmdPipelineBarrier2KHR(cmd, &dependencies);
#endif
}

VkImageSubresourceRange Image::subresourceRange(VkImageAspectFlags aspectMask)
{
    VkImageSubresourceRange range {};
    range.aspectMask = aspectMask;
    range.baseMipLevel = 0;
    range.levelCount = VK_REMAINING_MIP_LEVELS;
    range.baseArrayLayer = 0;
    range.layerCount = VK_REMAINING_ARRAY_LAYERS;
    return range;
}

void Image::cmdCopy(
    VkCommandBuffer cmd,
    VkImage srcImage,
    VkExtent2D srcSize,
    VkImage dstImage,
    VkExtent2D dstSize
)
{
    VkImageBlit2 blitRegion = {};
    blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;
	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo = {};
    blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    
	blitInfo.dstImage = dstImage;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = srcImage;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

#ifdef MINI_ENGINE_IS_WINDOWS
	vkCmdBlitImage2(cmd, &blitInfo);
#else
    vkCmdBlitImage2KHR(cmd, &blitInfo);
#endif
}

Image* Image::createAllocatedImage(
    VulkanData * vulkanData,
    VkFormat format,
    VkExtent2D extent,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspectFlags
)
{
    auto result = new Image();
    result->_vulkanData = vulkanData;
    result->_extent = { extent.width, extent.height, 1 };
    result->_format = format;
    
    auto imgInfo = Init::imageCreateInfo(
        result->_format,
        usage,
        result->_extent
    );
    
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    vmaCreateImage(
        vulkanData->allocator(),
        &imgInfo,
        &allocInfo,
        &result->_image,
        &result->_allocation,
        nullptr
    );
    
    auto imgViewInfo = Init::imageViewCreateInfo(format, result->_image, aspectFlags);
    VK_ASSERT(vkCreateImageView(vulkanData->device(), &imgViewInfo, nullptr, &result->_imageView));
    
    return result;
}

void Image::cleanup()
{
    vkDestroyImageView(_vulkanData->device(), _imageView, nullptr);
    vmaDestroyImage(_vulkanData->allocator(), _image, _allocation);
}

}
}
