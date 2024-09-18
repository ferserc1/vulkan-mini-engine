#pragma once

#include <mini_engine/core/common.hpp>
#include <mini_engine/VulkanData.hpp>

namespace miniengine {
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

    VkShaderModule _shaderModule = VK_NULL_HANDLE;
    std::string _shaderEntryPoint = "main";
    VkPipelineShaderStageCreateInfo _shaderStageInfo = {};
};

}
}
