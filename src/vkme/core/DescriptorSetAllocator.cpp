
#include <vkme/core/DescriptorSetAllocator.hpp>

namespace vkme {
namespace core {

void DescriptorSetAllocator::init(VulkanData * vulkanData)
{
    _vulkanData = vulkanData;
}

void DescriptorSetAllocator::initPool(
    VkDevice device,
    uint32_t maxSets,
    std::vector<PoolSizeRatio> poolRatios
) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (auto ratio : poolRatios)
    {
        VkDescriptorPoolSize size = {};
        size.type = ratio.type;
        size.descriptorCount = uint32_t(ratio.ratio * maxSets);
        poolSizes.push_back(size);
    }
    VkDescriptorPoolCreateInfo  poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = 0;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = uint32_t(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    vkCreateDescriptorPool(_vulkanData->device(), &poolInfo, nullptr, &_pool);
}

void DescriptorSetAllocator::clearDescriptors()
{
    vkResetDescriptorPool(_vulkanData->device(), _pool, 0);
}

void DescriptorSetAllocator::destroy()
{
    vkDestroyDescriptorPool(_vulkanData->device(), _pool, nullptr);
}

VkDescriptorSet DescriptorSetAllocator::allocateRaw(VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    VkDescriptorSet descriptorSet;
    VK_ASSERT(vkAllocateDescriptorSets(_vulkanData->device(), &allocInfo, &descriptorSet));
    return descriptorSet;
}

DescriptorSet * DescriptorSetAllocator::allocate(VkDescriptorSetLayout layout)
{
    auto dsWrapper = new DescriptorSet();
    dsWrapper->init(_vulkanData, allocateRaw(layout));
    return dsWrapper;
}

}
}
