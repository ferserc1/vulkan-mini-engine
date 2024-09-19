#pragma once

#include <vkme/core/common.hpp>
#include <vector>

namespace vkme {
namespace factory {

class DescriptorSetLayout {
public:
    void addBinding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(
        VkDevice device,
        VkShaderStageFlags shaderStages,
        void* pNext = nullptr,
        VkDescriptorSetLayoutCreateFlags flags = 0
    );

protected:
    std::vector<VkDescriptorSetLayoutBinding> _bindings;
};

}
}
