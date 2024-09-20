
#include <vkme/factory/ShaderModule.hpp>

#include <vkme/PlatformTools.hpp>

#include <fstream>

namespace vkme {
namespace factory {

VkShaderModule ShaderModule::loadFromSPV(const std::string& fileName, VkDevice device, const std::string& basePath)
{
    std::string shaderPath = (basePath.size() == 0 ? PlatformTools::shaderPath() : basePath) + fileName;
    std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);
    
    if (!file.is_open())
    {
        throw new std::runtime_error("Shader file not found at path " + shaderPath);
    }
    
    size_t fileSize = size_t(file.tellg());
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    
    VkShaderModuleCreateInfo info = {};
    info.pNext = nullptr;
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = buffer.size() * sizeof(uint32_t);
    info.pCode = buffer.data();
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &info, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw new std::runtime_error("Error creating shader at path " + shaderPath);
    }
    return shaderModule;
}


}
}
