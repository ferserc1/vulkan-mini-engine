
#include <mini_engine/factory/DescriptorSetLayout.hpp>

namespace miniengine {
namespace factory {

void DescriptorSetLayout::addBinding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding bindingInfo = {};
    bindingInfo.binding = binding;
    bindingInfo.descriptorCount = 1;
    bindingInfo.descriptorType = type;
    _bindings.push_back(bindingInfo);
}

void DescriptorSetLayout::clear()
{
    _bindings.clear();
}

VkDescriptorSetLayout DescriptorSetLayout::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& b : _bindings)
    {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = uint32_t(_bindings.size());
    layoutInfo.pBindings = _bindings.data();
    layoutInfo.flags = flags;
    VkDescriptorSetLayout set;
    VK_ASSERT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &set));
    return set;
}

}
}