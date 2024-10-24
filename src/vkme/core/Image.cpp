
#include <vkme/PlatformTools.hpp>
#include <vkme/core/Image.hpp>
#include <vkme/core/Info.hpp>
#include <vkme/core/Buffer.hpp>

#include <vkme/VulkanData.hpp>

#include <stb_image.h>

namespace vkme {
namespace core {

void Image::cmdTransitionImage(
    VkCommandBuffer       cmd,
    VkImage               image,
    VkImageLayout         oldLayout,
    VkImageLayout         newLayout,
    VkImageAspectFlags    aspectMask,
    VkPipelineStageFlags2 srcStageMask,
    VkAccessFlags2        srcAccessMask,
    VkPipelineStageFlags2 dstStageMask,
    VkAccessFlags2        dstAccessMask
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
    
    cmdPipelineBarrier2(cmd, &dependencies);
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

    cmdBlitImage2(cmd, &blitInfo);
}

Image* Image::createAllocatedImage(
    VulkanData * vulkanData,
    VkFormat format,
    VkExtent2D extent,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspectFlags,
    uint32_t arrayLayers
)
{
    auto result = new Image();
    result->_vulkanData = vulkanData;
    result->_extent = { extent.width, extent.height, 1 };
    result->_format = format;
    
    auto imgInfo = Info::imageCreateInfo(
        result->_format,
        usage,
        result->_extent,
        arrayLayers
    );

    if (arrayLayers == 6)
    {
        imgInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
    
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    VmaAllocator alloc = vulkanData->allocator();
    vmaCreateImage(
        alloc,
        &imgInfo,
        &allocInfo,
        &result->_image,
        &result->_allocation,
        nullptr
    );
    
    auto imgViewInfo = Info::imageViewCreateInfo(format, result->_image, aspectFlags);
    if (arrayLayers == 6)
    {
        imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        imgViewInfo.subresourceRange.layerCount = 6;
    }
    VK_ASSERT(vkCreateImageView(vulkanData->device(), &imgViewInfo, nullptr, &result->_imageView));
    
    return result;
}

Image* Image::createAllocatedImage(
    VulkanData * vulkanData,
    void* data,
    VkExtent2D extent,
    uint32_t dataBytesPerPixel,  // WARNING: for now, it only works with 4 bpp
    VkFormat imageFormat,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspectFlags
) {
    size_t dataSize = extent.width * extent.height * dataBytesPerPixel;
    auto uploadBuffer = std::unique_ptr<Buffer>(
        Buffer::createAllocatedBuffer(
            vulkanData,
            dataSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        )
    );
    
    auto uploadData = uploadBuffer->allocatedData();
    memcpy(uploadData, data, dataSize);
    
    auto image = createAllocatedImage(
        vulkanData,
        imageFormat,
        extent,
        usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        aspectFlags
    );
    
    vulkanData->command().immediateSubmit([&](VkCommandBuffer cmd) {
        Image::cmdTransitionImage(
            cmd,
            image->image(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );
        
        VkBufferImageCopy copyRgn = {};
        copyRgn.imageSubresource.aspectMask = aspectFlags;
        copyRgn.imageSubresource.mipLevel = 0;
        copyRgn.imageSubresource.baseArrayLayer = 0;
        copyRgn.imageSubresource.layerCount = 1;
        copyRgn.imageExtent = VkExtent3D { extent.width, extent.height, 1 };
        
        vkCmdCopyBufferToImage(
            cmd,
            uploadBuffer->buffer(),
            image->image(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRgn
        );
        
        Image::cmdTransitionImage(
            cmd,
            image->image(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
    });
    
    uploadBuffer->cleanup();
    
    return image;
}

Image* Image::wrapSwapchainImage(
    const Swapchain* swapchain,
    uint32_t swapchainImageIndex
) {
    auto result = new Image();
    
    result->_image = swapchain->image(swapchainImageIndex);
    result->_imageView = swapchain->imageView(swapchainImageIndex);
    result->_format = swapchain->imageFormat();
    result->_extent = VkExtent3D{
        swapchain->extent().width,
        swapchain->extent().height,
        1
    };

    return result;
}

Image* Image::loadImage(
    VulkanData * vulkanData,
    const std::string& filePath,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspectFlags
) {
    int width, height, channels;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
    if (!data) {
        throw std::runtime_error("Error loading image at path " + filePath);
    }
    
    VkExtent2D extent { uint32_t(width), uint32_t(height) };
    auto result = Image::createAllocatedImage(vulkanData, data, extent, 4, VK_FORMAT_R8G8B8A8_UNORM, usage, aspectFlags);
    return result;
}

void Image::cleanup()
{
    vkDestroyImageView(_vulkanData->device(), _imageView, nullptr);
    vmaDestroyImage(_vulkanData->allocator(), _image, _allocation);
}

}
}
