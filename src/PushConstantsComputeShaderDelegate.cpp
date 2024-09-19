#include <PushConstantsComputeShaderDelegate.hpp>

#include <vkme/factory/DescriptorSetLayout.hpp>
#include <vkme/factory/ComputePipeline.hpp>

void PushConstantsComputeShaderDelegate::init(vkme::VulkanData * vulkanData)
{
    _vulkanData = vulkanData;
    
    _drawImage = std::shared_ptr<vkme::core::Image>(vkme::core::Image::createAllocatedImage(
        vulkanData,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        vulkanData->swapchain().extent(),
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    ));
    
    vulkanData->cleanupManager().push([this] {
        _drawImage->cleanup();
    });
    
    initDescriptors();
    
    initPipelines();
}

void PushConstantsComputeShaderDelegate::init(vkme::VulkanData * vulkanData, vkme::UserInterface * ui)
{
    _vulkanData = vulkanData;
}

VkImageLayout PushConstantsComputeShaderDelegate::draw(VkCommandBuffer cmd, VkImage swapchainImage, VkExtent2D imageExtent, uint32_t currentFrame)
{
    using namespace vkme;
    
    // Transition draw image to render on it
    core::Image::cmdTransitionImage(
        cmd,
        _drawImage->image(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL
    );
    
    drawBackground(cmd, currentFrame, imageExtent);
    
    // Transition _drawImage and swapchain image to copy the first one to the second one
    core::Image::cmdTransitionImage(
        cmd,
        _drawImage->image(),
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );
    core::Image::cmdTransitionImage(
        cmd,
        swapchainImage,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    
    // Copy drawImage into swapchain image
    core::Image::cmdCopy(
        cmd,
        _drawImage->image(), _drawImage->extent2D(),
        swapchainImage, imageExtent
    );
    
    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
}

void PushConstantsComputeShaderDelegate::drawUI()
{
    if (ImGui::Begin("Background"))
    {
        ComputeEffect& selected = _backgroundEffect[_currentBackgroundEffect];
        
        ImGui::Text("Selected effect: %s", selected.name.c_str());
        
        ImGui::SliderInt("Effect Index", &_currentBackgroundEffect, 0, int(_backgroundEffect.size() - 1));
        
        ImGui::InputFloat4("Data 1", reinterpret_cast<float*>(&selected.data.data1));
        ImGui::InputFloat4("Data 2", reinterpret_cast<float*>(&selected.data.data2));
    }
    ImGui::End();
}

void PushConstantsComputeShaderDelegate::drawBackground(VkCommandBuffer cmd, uint32_t currentFrame, VkExtent2D imageExtent)
{
    VkPipeline pl = _backgroundEffect[_currentBackgroundEffect].pipeline;
    VkPipelineLayout layout = _backgroundEffect[_currentBackgroundEffect].layout;
    auto &pc = _backgroundEffect[_currentBackgroundEffect].data;
    
    // We are using data4 to pass frame relative variables
    pc.data4.x = std::abs(std::sin(glm::radians(static_cast<float>(currentFrame % 360))));
    pc.data4.y = float(currentFrame);
    
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pl);
    
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        layout, 0, 1,
        &(*_drawImageDescriptors), 0, nullptr
    );
    
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &pc);
    
    vkCmdDispatch(cmd, std::ceil(imageExtent.width / 16.0), std::ceil(imageExtent.height / 16.0), 1);
}



void PushConstantsComputeShaderDelegate::initDescriptors()
{
    using namespace vkme::core;
    std::vector<DescriptorSetAllocator::PoolSizeRatio> sizes = {};

    // One image to pass the draw image to the compute shader
    sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 });
    
    _descriptorAllocator.init(_vulkanData);
    _descriptorAllocator.initPool(_vulkanData->device(), 10, sizes);
    
    vkme::factory::DescriptorSetLayout dsFactory;
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    _drawImageDescriptorLayout = dsFactory.build(_vulkanData->device(), VK_SHADER_STAGE_COMPUTE_BIT);
    
    // Wrapped method: using a vkme::core::DescriptorSet wrapper
    _drawImageDescriptors = std::unique_ptr<DescriptorSet>(_descriptorAllocator.allocate(_drawImageDescriptorLayout));
    _drawImageDescriptors->updateImage(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, _drawImage->imageView(), VK_IMAGE_LAYOUT_GENERAL);
    
    _vulkanData->cleanupManager().push([&] {
        _descriptorAllocator.destroy();
        
        vkDestroyDescriptorSetLayout(_vulkanData->device(), _drawImageDescriptorLayout, nullptr);
    });
}

void PushConstantsComputeShaderDelegate::initPipelines()
{
    using namespace vkme;
    
    VkPipeline pipeline;
    
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pSetLayouts = &_drawImageDescriptorLayout;
    layoutInfo.setLayoutCount = 1;
    
    VkPushConstantRange range = {};
    range.offset = 0;
    range.size = sizeof(ComputePushConstants);
    range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    layoutInfo.pPushConstantRanges = &range;
    layoutInfo.pushConstantRangeCount = 1;
    
    // All the shaders uses the same layout, so we'll only create one
    VK_ASSERT(vkCreatePipelineLayout(
        _vulkanData->device(),
        &layoutInfo,
        nullptr,
        &_pipelineLayout
    ));
    

    factory::ComputePipeline pipelineFactory(_vulkanData);
    pipelineFactory.setShader("gradient_color.comp.spv");
    pipeline = pipelineFactory.build(_pipelineLayout);
    _backgroundEffect.push_back({
        "color gradient",
        pipeline,
        _pipelineLayout,
        {
            { 1.0f, 0.0f, 0.0f, 1.0f },
            { 1.0f, 1.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f }
        }
    });

    pipelineFactory.setShader("gradient.comp.spv");
    pipeline = pipelineFactory.build(_pipelineLayout);
    _backgroundEffect.push_back({
        "color grid",
        pipeline,
        _pipelineLayout,
        {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f }
        }
    });
    
    pipelineFactory.setShader("sky.comp.spv");
    pipeline = pipelineFactory.build(_pipelineLayout);
    _backgroundEffect.push_back({
        "sky",
        pipeline,
        _pipelineLayout,
        {
            { 0.1f, 0.42f, 0.69f, 0.99f },
            { 0.0f, 30.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f }
        }
    });
    
    _currentBackgroundEffect = 2;
    
    _vulkanData->cleanupManager().push([&]() {
        vkDestroyPipelineLayout(_vulkanData->device(), _pipelineLayout, nullptr);
        for (auto &effect : _backgroundEffect) {
            vkDestroyPipeline(_vulkanData->device(), effect.pipeline, nullptr);
        }
    });
    
}
