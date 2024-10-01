
#include <vkme/core/DescriptorSetAllocator.hpp>

namespace vkme {
namespace core {

void DescriptorSetAllocator::init(VulkanData * vulkanData)
{
    _vulkanData = vulkanData;
}

void DescriptorSetAllocator::initPool(
    uint32_t maxSets,
    std::vector<PoolSizeRatio> poolRatios
) {
    _poolSizes.clear();
    for (auto ratio : poolRatios)
    {
        VkDescriptorPoolSize size = {};
        size.type = ratio.type;
        size.descriptorCount = uint32_t(ratio.ratio * maxSets);
        _poolSizes.push_back(size);
    }
    
    VkDescriptorPool newPool = createPool(maxSets);
    _setsPerPool = uint32_t(std::round(float(maxSets) * 1.5f));
    _readyPools.push_back(newPool);
}

VkDescriptorPool DescriptorSetAllocator::getPool()
{
    VkDescriptorPool newPool;
    if (_readyPools.size() != 0)
    {
        newPool = _readyPools.back();
        _readyPools.pop_back();
    }
    else
    {
        newPool = createPool(_setsPerPool);
        
        _setsPerPool = uint32_t(std::round(float(_setsPerPool) * 1.5f));
        if (_setsPerPool > 4096) {
            _setsPerPool = 4096;
        }
    }
    return newPool;
}

VkDescriptorPool DescriptorSetAllocator::createPool(uint32_t setCount)
{
    VkDescriptorPoolCreateInfo  poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = setCount;
    poolInfo.poolSizeCount = uint32_t(_poolSizes.size());
    poolInfo.pPoolSizes = _poolSizes.data();

    VkDescriptorPool pool;
    vkCreateDescriptorPool(_vulkanData->device(), &poolInfo, nullptr, &pool);
    return pool;
}

void DescriptorSetAllocator::clearDescriptors()
{
    for (auto p : _readyPools)
    {
        vkResetDescriptorPool(_vulkanData->device(), p, 0);
    }
    for (auto p : _fullPools)
    {
        vkResetDescriptorPool(_vulkanData->device(), p, 0);
        _readyPools.push_back(p);
    }
    _fullPools.clear();
}

void DescriptorSetAllocator::destroy()
{
    for (auto p : _readyPools)
    {
        vkDestroyDescriptorPool(_vulkanData->device(), p, nullptr);
    }
    _readyPools.clear();
    for (auto p : _fullPools)
    {
        vkDestroyDescriptorPool(_vulkanData->device(), p, nullptr);
    }
    _fullPools.clear();
}

VkDescriptorSet DescriptorSetAllocator::allocateRaw(VkDescriptorSetLayout layout, void* pNext)
{
    VkDescriptorPool poolToUse = getPool();
    
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = pNext;
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    
    VkDescriptorSet descriptorSet;
    auto result = vkAllocateDescriptorSets(_vulkanData->device(), &allocInfo, &descriptorSet);
    
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        _fullPools.push_back(poolToUse);
        
        poolToUse = getPool();
        allocInfo.descriptorPool = poolToUse;
        
        if (vkAllocateDescriptorSets(_vulkanData->device(), &allocInfo, &descriptorSet) != VK_SUCCESS)
        {
            throw new std::runtime_error("Could not allocate descriptor set. Review the configuration of the descriptor pool ratios in initPools() call. Make sure you have included all the descriptor types you are going to use in the descriptor set layout.");
        }
    }
    
    _readyPools.push_back(poolToUse);
    return descriptorSet;
}

DescriptorSet * DescriptorSetAllocator::allocate(VkDescriptorSetLayout layout, void* pNext)
{
    auto dsWrapper = new DescriptorSet();
    dsWrapper->init(_vulkanData, allocateRaw(layout, pNext));
    return dsWrapper;
}

}
}
