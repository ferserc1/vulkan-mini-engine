#pragma once

#include <vkme/core/common.hpp>
#include <vkme/VulkanData.hpp>
#include <vkme/core/DescriptorSet.hpp>

namespace vkme {
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

    VkDescriptorSet allocateRaw(VkDescriptorSetLayout layout, void* pNext = nullptr);
    
    DescriptorSet * allocate(VkDescriptorSetLayout layout, void* pNext = nullptr);

protected:
    std::vector<VkDescriptorPoolSize> _poolSizes;
    std::vector<VkDescriptorPool> _fullPools;
    std::vector<VkDescriptorPool> _readyPools;
    uint32_t _setsPerPool;
    //VkDescriptorPool _pool;
    
    VkDescriptorPool getPool();
    VkDescriptorPool createPool(uint32_t setCount);

    VulkanData * _vulkanData;
    
};

}
}
