#include <vkme/factory/GraphicsPipeline.hpp>
#include <vkme/factory/ShaderModule.hpp>

namespace vkme {
namespace factory {

GraphicsPipeline::GraphicsPipeline(VulkanData * vulkanData)
    :_vulkanData(vulkanData)
{
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    colorBlendAttachment = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    _renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    _renderInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    
    // Default values
    setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    setPolygonMode(VK_POLYGON_MODE_FILL);
    setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    disableMultisample();
    disableBlending();
    disableDepthtest();
}

GraphicsPipeline::~GraphicsPipeline()
{
    clearShaders();
}

void GraphicsPipeline::addShader(const std::string& fileName, VkShaderStageFlagBits stage, const std::string& entryPoint, const std::string& basePath)
{
    auto shaderModule = factory::ShaderModule::loadFromSPV(fileName, _vulkanData->device(), basePath);
    addShader(shaderModule, stage, entryPoint);
}

void GraphicsPipeline::addShader(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const std::string& entryPoint)
{
    _shaders.push_back({
        shaderModule,
        stage,
        entryPoint
    });
}

void GraphicsPipeline::clearShaders()
{
    for (auto& shaderData : _shaders)
    {
        vkDestroyShaderModule(_vulkanData->device(), shaderData.shaderModule, nullptr);
    }
    _shaders.clear();
}

void GraphicsPipeline::setInputTopology(VkPrimitiveTopology topology)
{
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void GraphicsPipeline::setPolygonMode(VkPolygonMode mode, float lineWidth)
{
    rasterizer.polygonMode = mode;
    rasterizer.lineWidth = 1.f;
}

void GraphicsPipeline::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = frontFace;
}

void GraphicsPipeline::disableMultisample()
{
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
}

void GraphicsPipeline::setColorAttachmentFormat(VkFormat format, uint32_t viewMask)
{
    _colorAttachmentformat = format;
    _renderInfo.colorAttachmentCount = 1;
    _renderInfo.pColorAttachmentFormats = &_colorAttachmentformat;
    _renderInfo.viewMask = viewMask;
}

void GraphicsPipeline::setDepthFormat(VkFormat format)
{
    _renderInfo.depthAttachmentFormat = format;
}

void GraphicsPipeline::disableDepthtest()
{
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};
    depthStencil.minDepthBounds = 0.f;
    depthStencil.maxDepthBounds = 1.f;
}

void GraphicsPipeline::enableDepthtest(bool depthWriteEnable, VkCompareOp op) {
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = depthWriteEnable;
    depthStencil.depthCompareOp = op;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
}

void GraphicsPipeline::disableBlending()
{
    colorBlendAttachment.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | 
        VK_COLOR_COMPONENT_G_BIT | 
        VK_COLOR_COMPONENT_B_BIT | 
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
}

void GraphicsPipeline::enableBlendingAdditive()
{
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void GraphicsPipeline::enableBlendingAlphablend()
{
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

VkPipeline GraphicsPipeline::build(VkPipelineLayout layout)
{
    VkPipelineViewportStateCreateInfo viewportInfo = {};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    for (auto &shaderData : _shaders)
    {
        stages.push_back({
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,                                  // flags;
            shaderData.stage,                   // stage;
            shaderData.shaderModule,            // module;
            shaderData.entryPoint.c_str(),      // pName;
            nullptr                             // pSpecializationInfo;
        });
    }
    pipelineInfo.pStages = stages.data();
    pipelineInfo.stageCount = uint32_t(stages.size());
    pipelineInfo.pVertexInputState = &vertexInputState;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = layout;
    
    // In dynamic rendering mode, we use pNext to store the color and depth attachments formats
    pipelineInfo.pNext = &_renderInfo;
    
    VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    
    VkPipelineDynamicStateCreateInfo dynamicInfo = {};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.pDynamicStates = &dynamicState[0];
    dynamicInfo.dynamicStateCount = 2;
    pipelineInfo.pDynamicState = &dynamicInfo;
    
    VkPipeline pipeline;
    VK_ASSERT(vkCreateGraphicsPipelines(_vulkanData->device(), nullptr, 1, &pipelineInfo, nullptr, &pipeline));
    return pipeline;
}

}
}
