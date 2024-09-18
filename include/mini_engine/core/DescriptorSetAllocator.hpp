#pragma once

#include <mini_engine/core/common.hpp>
#include <mini_engine/VulkanData.hpp>
#include <mini_engine/core/DescriptorSet.hpp>

namespace miniengine {
namespace core {

class DescriptorSetAllocator {
public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    void init(VulkanData * vulkanData);

    void initPool(
        VkDevice device,
        uint32_t maxSets,
        std::vector<PoolSizeRatio> poolRatios
    );

    void clearDescriptors();

    void destroy();

    VkDescriptorSet allocateRaw(VkDescriptorSetLayout layout);
    
    DescriptorSet * allocate(VkDescriptorSetLayout layout);

protected:
    VkDescriptorPool _pool;

    VulkanData * _vulkanData;
};

}
}
