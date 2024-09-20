#pragma once

#include <vkme/core/common.hpp>
#include <vkme/VulkanData.hpp>

namespace vkme {
namespace factory {

class ComputePipeline {
public:
    ComputePipeline(VulkanData * vulkanData);
    ~ComputePipeline();

    void setShader(const std::string& fileName, const std::string& entryPoint = "main", const std::string& basePath = "");
    void setShader(VkShaderModule shaderModule, const std::string& entryPoint = "main");

    VkPipeline build(VkPipelineLayout layout);

protected:
    VulkanData * _vulkanData;

    VkShaderModule _shaderModule;
    std::string _shaderEntryPoint = "main";
    VkPipelineShaderStageCreateInfo _shaderStageInfo = {};
};

}
}
