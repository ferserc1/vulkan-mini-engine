#pragma once

#include <vkme/core/common.hpp>
#include <vkme/VulkanData.hpp>


namespace vkme {
namespace factory {

class Sampler {
public:
    Sampler(VulkanData * vulkanData);
    
    VkSamplerCreateInfo createInfo;

    VkSampler build(
        VkFilter magFilter = VK_FILTER_LINEAR,
        VkFilter minFilter = VK_FILTER_LINEAR,
        VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT
    );
    
protected:
    VulkanData * _vulkanData;
};

}
}
