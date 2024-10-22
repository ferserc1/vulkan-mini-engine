#include <vkme/tools/SkyboxRenderer.hpp>
#include <vkme/factory/DescriptorSetLayout.hpp>
#include <vkme/factory/GraphicsPipeline.hpp>
#include <vkme/factory/Sampler.hpp>
#include <vkme/core/Info.hpp>
#include <vkme/geo/Cube.hpp>

namespace vkme::tools {

SkyboxRenderer::SkyboxRenderer(VulkanData* vulkanData, core::DescriptorSetAllocator* dsAllocator)
    :_vulkanData(vulkanData)
    ,_descriptorSetAllocator(dsAllocator)
{

}
    
void SkyboxRenderer::getFrameResourcesRequirements(std::vector<core::DescriptorSetAllocator::PoolSizeRatio>& requiredRatios)
{
    requiredRatios.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 });
}

void SkyboxRenderer::init(std::shared_ptr<core::Image>&& skyImage)
{
    _skyImage = skyImage;
    
    vkme::factory::Sampler sampler(_vulkanData);
    _imageSampler = sampler.build();
    
    // Pipeline and Pipeline layout
    vkme::factory::DescriptorSetLayout dsFactory;
    
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    _uniformBufferDSLayout = dsFactory.build(
        _vulkanData->device(),
        VK_SHADER_STAGE_VERTEX_BIT
    );
    
    dsFactory.clear();
    dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    _inputImageDSLayout = dsFactory.build(
       _vulkanData->device(),
       VK_SHADER_STAGE_FRAGMENT_BIT
    );
    
    VkPushConstantRange bufferRange = {};
    bufferRange.offset = 0;
    bufferRange.size = sizeof(vkme::geo::MeshPushConstants);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    auto layoutInfo = vkme::core::Info::pipelineLayoutInfo();
    layoutInfo.pPushConstantRanges = &bufferRange;
    layoutInfo.pushConstantRangeCount = 1;
    VkDescriptorSetLayout layouts[] = {
        _uniformBufferDSLayout,
        _inputImageDSLayout
    };
    layoutInfo.pSetLayouts = layouts;
    layoutInfo.setLayoutCount = 2;
    
    VK_ASSERT(vkCreatePipelineLayout(_vulkanData->device(), &layoutInfo, nullptr, &_pipelineLayout));
    
    vkme::factory::GraphicsPipeline plFactory(_vulkanData);
    
    plFactory.addShader("sky_cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("sky_cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    plFactory.setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
    plFactory.setDepthFormat(_vulkanData->swapchain().depthImageFormat());
    plFactory.disableDepthtest();
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // TODO: Optimize this
    plFactory.setCullMode(false, VK_FRONT_FACE_CLOCKWISE);
    //plFactory.setCullMode(true, VK_FRONT_FACE_CLOCKWISE);
    
    _pipeline = plFactory.build(_pipelineLayout);
    
    _vulkanData->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _uniformBufferDSLayout, nullptr);
        vkDestroyDescriptorSetLayout(dev, _inputImageDSLayout, nullptr);
    });
    
    // Sky cube
    _skyCube = std::shared_ptr<vkme::geo::Model>(vkme::geo::Cube::createCube(_vulkanData, 1.0f, "Sky Cube", {
        std::shared_ptr<vkme::geo::Modifier>(new vkme::geo::FlipFacesModifier()),
        
        // This one is actually not needed, because the sky shader does not do lighting calculations.
        std::shared_ptr<vkme::geo::Modifier>(new vkme::geo::FlipNormalsModifier())
    }));
    
    _skyCube->allocateMaterialDescriptorSets(_descriptorSetAllocator, _inputImageDSLayout);
    
    _skyCube->updateDescriptorSets([&](vkme::core::DescriptorSet* ds) {
        ds->updateImage(
            0,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            _skyImage->imageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            _imageSampler
        );
    });
    
    _vulkanData->cleanupManager().push([&](VkDevice) {
        _skyCube->cleanup();
    });
}

void SkyboxRenderer::update(const glm::mat4& view, const glm::mat4& proj)
{
    _skyData.view = view;
    _skyData.proj = proj;
}

void SkyboxRenderer::draw(VkCommandBuffer cmd, uint32_t currentFrame, vkme::core::FrameResources& frameResources)
{
    auto uniformBuffer = vkme::core::Buffer::createAllocatedBuffer(
        _vulkanData,
        sizeof(SkyData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    
    SkyData * skyDataPtr = reinterpret_cast<SkyData*>(uniformBuffer->allocatedData());
    *skyDataPtr = _skyData;
    
    frameResources.cleanupManager.push([&, uniformBuffer](VkDevice) {
        uniformBuffer->cleanup();
        delete uniformBuffer;
    });
    
    auto descriptorSet = std::unique_ptr<vkme::core::DescriptorSet>(
        frameResources.descriptorAllocator->allocate(_uniformBufferDSLayout)
    );
    descriptorSet->updateBuffer(
        0,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        uniformBuffer,
        sizeof(SkyData),
        0
    );
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    
    vkme::core::DescriptorSet* ds[] = {
        descriptorSet.get()
    };
    _skyCube->draw(cmd, _pipelineLayout, ds, 1);
}

}
