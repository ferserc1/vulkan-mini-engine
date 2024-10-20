#include <vkme/UserInterface.hpp>
#include <vkme/core/Command.hpp>
#include <vkme/core/Info.hpp>
#include <vkme/PlatformTools.hpp>

namespace vkme {

void UserInterface::init(VulkanData* vulkanData)
{
    _vulkanData = vulkanData;

    initCommands();

    auto fenceInfo = core::Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_ASSERT(vkCreateFence(_vulkanData->device(), &fenceInfo, nullptr, &_uiFence));
    
    initImGui();
}

void UserInterface::processEvent(SDL_Event * event)
{
    ImGui_ImplSDL2_ProcessEvent(event);
}

void UserInterface::newFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (_delegate) {
        _delegate->drawUI();
    }

    ImGui::Render();
}

void UserInterface::draw(VkCommandBuffer cmd, VkImageView targetImageView)
{
    auto colorAttachment = core::Info::attachmentInfo(
        targetImageView,
        nullptr,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    auto extent = _vulkanData->swapchain().extent();
    auto renderingInfo = core::Info::renderingInfo(
        extent,
        &colorAttachment,
        nullptr
    );
    
    vkme::core::cmdBeginRendering(cmd, &renderingInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    
    vkme::core::cmdEndRendering(cmd);
}

void UserInterface::cleanup()
{
    
}

void UserInterface::initCommands()
{
    _commandPool = _vulkanData->command().createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    _commandBuffer = _vulkanData->command().allocateCommandBuffer(_commandPool, 1);

    _vulkanData->cleanupManager().push([&](VkDevice dev){
        vkDestroyFence(dev, _uiFence, nullptr);
        _vulkanData->command().destroyComandPool(_commandPool);
    });
}

void UserInterface::initImGui()
{
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = uint32_t(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;
    
    VK_ASSERT(vkCreateDescriptorPool(_vulkanData->device(), &poolInfo, nullptr, &_imguiPool));
    
    ImGui::CreateContext();
    
    SDL_Window * window = _vulkanData->window();
    ImGui_ImplSDL2_InitForVulkan(window);
    
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = _vulkanData->instance();
    initInfo.PhysicalDevice = _vulkanData->physicalDevice();
    initInfo.Device = _vulkanData->device();
    initInfo.Queue = _vulkanData->command().graphicsQueue();
    initInfo.DescriptorPool = _imguiPool;
    initInfo.MinImageCount = 3;
    initInfo.ImageCount = 3;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo = {};
    initInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    VkFormat format = _vulkanData->swapchain().imageFormat();
    initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    
    ImGui_ImplVulkan_Init(&initInfo);
    
    _vulkanData->cleanupManager().push([&, this](VkDevice dev) {
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(dev, _imguiPool, nullptr);
    });
}

}
