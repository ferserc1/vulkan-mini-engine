#pragma once

#include <mini_engine/core/common.hpp>
#include <string>

namespace miniengine {
namespace factory {

class ShaderModule
{
public:
    // If the basePath is not specified, the shader will be loaded from the
    // default shader path (see PlatformTools::shaderPath())
    static VkShaderModule loadFromSPV(
        const std::string& fileName,
        VkDevice device,
        const std::string& basePath = ""
    );
};

}
}
