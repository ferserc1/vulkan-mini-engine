
#include <vkme/factory/ComputePipeline.hpp>
#include <vkme/factory/ShaderModule.hpp>

namespace vkme {
namespace factory {

ComputePipeline::ComputePipeline(VulkanData * vulkanData)
    :_vulkanData(vulkanData)
{
}

ComputePipeline::~ComputePipeline()
{
    if (_shaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(_vulkanData->device(), _shaderModule, nullptr);
    }
}

void ComputePipeline::setShader(const std::string& fileName, const std::string& entryPoint,  const std::string& basePath)
{
    _shaderModule = ShaderModule::loadFromSPV(fileName, _vulkanData->device(), basePath);
    setShader(_shaderModule, entryPoint);
}

void ComputePipeline::setShader(VkShaderModule shaderModule, const std::string& entryPoint)
{
    if (_shaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(_vulkanData->device(), _shaderModule, nullptr);
    }
    
    _shaderEntryPoint = entryPoint;
    _shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    _shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    _shaderStageInfo.module = shaderModule;
    _shaderStageInfo.pName = _shaderEntryPoint.c_str();
}

VkPipeline ComputePipeline::build(VkPipelineLayout layout)
{
    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = _shaderStageInfo;
    pipelineInfo.layout = layout;

    VkPipeline pipeline;
    VK_ASSERT(vkCreateComputePipelines(
        _vulkanData->device(),
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        nullptr,
        &pipeline
    ));
    return pipeline;
}

}
}
