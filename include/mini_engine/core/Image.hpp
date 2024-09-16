
#pragma once

#include <mini_engine/core/common.hpp>

namespace miniengine {
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
    static void transitionImage(
        VkCommandBuffer       cmd,
        VkImage               image,
        VkImageLayout         oldLayout,
        VkImageLayout         newLayout,
        VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VkAccessFlags2        srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VkAccessFlags2        dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
    );

    /*
     *  Returns a default initialised VkImageSubresourceRange structure for all mipmaps and layers.
     */
    static VkImageSubresourceRange subresourceRange(VkImageAspectFlags aspectMask);
};

}
}
