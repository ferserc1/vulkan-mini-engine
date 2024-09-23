
#include <vkme/core/common.hpp>
#include <vkme/VulkanData.hpp>

#include <vector>

namespace vkme {
namespace factory {

class GraphicsPipeline {
public:
    GraphicsPipeline(VulkanData * vulkanData);
    ~GraphicsPipeline();
    
    void addShader(const std::string& fileName, VkShaderStageFlagBits stage, const std::string& entryPoint = "main", const std::string& basePath = "");
    void addShader(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const std::string& entryPoint = "main");
    void clearShaders();

    void setInputTopology(VkPrimitiveTopology topology);
    void setPolygonMode(VkPolygonMode mode, float lineWidth = 1.0f);
    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);

    void disableMultisample();
    void setColorAttachmentFormat(VkFormat format);
    void setDepthFormat(VkFormat format);
    void disableDepthtest();
    void enableDepthtest(bool depthWriteEnable, VkCompareOp op);
    void disableBlending();
    void enableBlendingAdditive();
    void enableBlendingAlphablend();
    
    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    
    VkPipeline build(VkPipelineLayout layout);
    
protected:
    VulkanData * _vulkanData;

    VkPipelineRenderingCreateInfo _renderInfo = {};
    VkFormat _colorAttachmentformat;
    
    struct ShaderData {
        VkShaderModule shaderModule;
        VkShaderStageFlagBits stage;
        std::string entryPoint;
    };
    
    std::vector<ShaderData> _shaders;
};

}
}
