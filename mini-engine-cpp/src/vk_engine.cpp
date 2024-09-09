//> includes
#include "vk_engine.h"

#include <iostream>
#include <fstream>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_initializers.h>
#include <vk_types.h>

#include "VkBootstrap.h"

#include <chrono>
#include <thread>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

VulkanEngine* loadedEngine = nullptr;

VulkanEngine& VulkanEngine::Get() { return *loadedEngine; }
void VulkanEngine::init()
{
    // only one engine initialization is allowed with the application.
    assert(loadedEngine == nullptr);
    loadedEngine = this;

    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        _windowExtent.width,
        _windowExtent.height,
        window_flags
    );
    
    init_vulkan();
    
    init_swapchain();
    
    init_commands();
    
    init_default_renderpass();
    
    init_framebuffers();
    
    init_sync_structures();
    
    init_pipelines();
    
    load_meshes();
    
    // everything went fine
    _isInitialized = true;
}

void VulkanEngine::cleanup()
{
    if (_isInitialized) {
        vkDeviceWaitIdle(_device);
        vkWaitForFences(_device, 1, &_renderFence, true, 1000000000);
        
        _mainDeletionQueue.flush();
        
        vkDestroyDevice(_device, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
        
        SDL_DestroyWindow(_window);
    }

    // clear engine pointer
    loadedEngine = nullptr;
}

void VulkanEngine::draw()
{
    VK_CHECK(vkWaitForFences(_device, 1, &_renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(_device, 1, &_renderFence));
    
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, _presentSemaphore, nullptr, &swapchainImageIndex));
    
    VK_CHECK(vkResetCommandBuffer(_mainCommandBuffer, 0));
    
    VkCommandBuffer cmd = _mainCommandBuffer;
    
    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    
    VkClearValue clearValue;
    float flash = abs(sin(_frameNumber / 120.0f));
    clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };
    
    VkRenderPassBeginInfo rpInfo = {};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext = nullptr;
    rpInfo.renderPass = _renderPass;
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent = _windowExtent;
    rpInfo.framebuffer = _framebuffers[swapchainImageIndex];
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearValue;
    
    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    //vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _selectedShader == 0 ? _trianglePipeline : _redTrianglePipeline);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipeline);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &_triangleMesh._vertexBuffer._buffer, &offset);
    vkCmdDraw(cmd, 3, 1, 0, 0);
    
    vkCmdEndRenderPass(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));
    
    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = nullptr;
    
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    
    submit.pWaitDstStageMask = &waitStage;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &_presentSemaphore;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &_renderSemaphore;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    
    VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _renderFence));
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &_renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;
    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));
    
    _frameNumber++;
    
}

static const int NUM_SHADERS = 2;

void VulkanEngine::run()
{
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT)
                bQuit = true;

            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    stop_rendering = true;
                }
                if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                    stop_rendering = false;
                }
            }
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_SPACE)
                {
                    _selectedShader += 1;
                    _selectedShader = _selectedShader % NUM_SHADERS;
                }
            }
        }

        // do not draw if we are minimized
        if (stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        draw();
    }
}

void VulkanEngine::init_vulkan()
{
    vkb::InstanceBuilder builder;
    
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
        .request_validation_layers(bUseValidationLayers)
        .use_default_debug_messenger()
        .require_api_version(1, 1, 0)
        .build();
    
    vkb::Instance vkb_inst = inst_ret.value();
    
    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;
    
    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);
    
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector
        .set_minimum_version(1, 1)
        .set_surface(_surface)
        .select()
        .value();
    
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();
    
    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;
    
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _chosenGPU;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    vmaCreateAllocator(&allocatorInfo, &_allocator);
}

void VulkanEngine::init_swapchain()
{
    create_swapchain(_windowExtent.width, _windowExtent.height);
}

void VulkanEngine::init_commands()
{
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    
    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_commandPool));
    
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_commandPool, 1);
    
    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_mainCommandBuffer));
    
    _mainDeletionQueue.push_function([=]() {
        vkDestroyCommandPool(_device, _commandPool, nullptr);
    });
}

void VulkanEngine::init_sync_structures()
{
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_renderFence));
    
    _mainDeletionQueue.push_function([=]() {
        vkDestroyFence(_device, _renderFence, nullptr);
    });
    
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_presentSemaphore));
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderSemaphore));
    
    _mainDeletionQueue.push_function([=]() {
        vkDestroySemaphore(_device, _presentSemaphore, nullptr);
        vkDestroySemaphore(_device, _renderSemaphore, nullptr);
    });
}

void VulkanEngine::init_default_renderpass()
{
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = _swapchainImageFormat;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    
    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    
    VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_renderPass));
    
    _mainDeletionQueue.push_function([=]() {
        vkDestroyRenderPass(_device, _renderPass, nullptr);
    });
}

void VulkanEngine::init_framebuffers()
{
    const uint32_t swapchain_imagecount = static_cast<uint32_t>(_swapchainImages.size());
    _framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);
    
    for (int i = 0; i < swapchain_imagecount; ++i) {
        VkImageView attachments[] = {
            _swapchainImageViews[i]
        };
        
        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.pNext = nullptr;
        fb_info.renderPass = _renderPass;
        fb_info.attachmentCount = 1;
        fb_info.pAttachments = attachments;
        fb_info.width = _windowExtent.width;
        fb_info.height = _windowExtent.height;
        fb_info.layers = 1;
        
        VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));
        
        _mainDeletionQueue.push_function([=]() {
            vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
            vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
        });
    }
}

void VulkanEngine::init_pipelines()
{
#if defined(MINI_ENGINE_IS_MAC)
    std::string shaderPath = PlatformUtils::macos_get_resources_path() + "shaders/";
#else
    std::string shaderPath = "../../shaders/";
#endif

    VkShaderModule redTriangleFragShader;
    if (!load_shader_module(std::string(shaderPath + "triangle.frag.spv").c_str(), &redTriangleFragShader))
    {
        std::cerr << "Error when building the red triangle fragment shader module" << std::endl;
    }
    else
    {
        std::cout << "Red triangle fragment shader successfully loaded" << std::endl;
    }
    
    VkShaderModule redTriangleVertShader;
    if (!load_shader_module(std::string(shaderPath + "triangle.vert.spv").c_str(), &redTriangleVertShader))
    {
        std::cerr << "Error when building the red triangle vertex shader module" << std::endl;
    }
    else
    {
        std::cout << "Red triangle vertex shader successfully loaded" << std::endl;
    }
    
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_trianglePipelineLayout));
    
    PipelineBuilder pipelineBuilder;
    
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, redTriangleVertShader));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, redTriangleFragShader));
    
    pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
    pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder._viewport.x = 0.0f;
    pipelineBuilder._viewport.y = 0.0f;
    pipelineBuilder._viewport.width = static_cast<float>(_windowExtent.width);
    pipelineBuilder._viewport.height = static_cast<float>(_windowExtent.height);
    pipelineBuilder._viewport.minDepth = 0.0f;
    pipelineBuilder._viewport.maxDepth = 1.0f;
    
    pipelineBuilder._scissor.offset = { 0, 0 };
    pipelineBuilder._scissor.extent = _windowExtent;
    
    pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    
    pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
    
    pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();
    
    pipelineBuilder._pipelineLayout = _trianglePipelineLayout;
    
    _redTrianglePipeline = pipelineBuilder.build_pipeline(_device, _renderPass);
    
    VkShaderModule triangleFragShader;
    if (!load_shader_module(std::string(shaderPath + "color_triangle.frag.spv").c_str(), &triangleFragShader))
    {
        std::cerr << "Error when building the triangle fragment shader module" << std::endl;
    }
    else
    {
        std::cout << "Triangle fragment shader successfully loaded" << std::endl;
    }
    
    VkShaderModule triangleVertShader;
    if (!load_shader_module(std::string(shaderPath + "color_triangle.vert.spv").c_str(), &triangleVertShader))
    {
        std::cerr << "Error when building the triangle vertex shader module" << std::endl;
    }
    else
    {
        std::cout << "Triangle vertex shader successfully loaded" << std::endl;
    }
    
    pipelineBuilder._shaderStages.clear();
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangleVertShader));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));
    
    _trianglePipeline = pipelineBuilder.build_pipeline(_device, _renderPass);
    
    VertexInputDescription vertexDescription = Vertex::get_vertex_description();
    
    pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
    pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
    
    pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
    pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();
    
    pipelineBuilder._shaderStages.clear();
    
    VkShaderModule meshVertShader;
    if (!load_shader_module(std::string(shaderPath + "tri_mesh.vert.spv").c_str(), &meshVertShader))
    {
        std::cerr << "Error when building the tri mesh vertex shader module" << std::endl;
    }
    else
    {
        std::cout << "Tri mesh vertex shader successfully loaded" << std::endl;
    }
    
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));
    
    _meshPipeline = pipelineBuilder.build_pipeline(_device, _renderPass);
    
    vkDestroyShaderModule(_device, meshVertShader, nullptr);
    vkDestroyShaderModule(_device, redTriangleFragShader, nullptr);
    vkDestroyShaderModule(_device, redTriangleVertShader, nullptr);
    vkDestroyShaderModule(_device, triangleVertShader, nullptr);
    vkDestroyShaderModule(_device, triangleFragShader, nullptr);
    
    _mainDeletionQueue.push_function([=]() {
        vkDestroyPipeline(_device, _redTrianglePipeline, nullptr);
        vkDestroyPipeline(_device, _trianglePipeline, nullptr);
        vkDestroyPipeline(_device, _meshPipeline, nullptr);
        
        vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
    });
}

void VulkanEngine::load_meshes()
{
    _triangleMesh._vertices.resize(3);
    
    _triangleMesh._vertices[0].position = { 1.0f, 1.0f, 0.0f };
    _triangleMesh._vertices[1].position = {-1.0f, 1.0f, 0.0f };
    _triangleMesh._vertices[2].position = { 0.0f,-1.0f, 0.0f };
    
    _triangleMesh._vertices[0].color = { 0.0f, 1.0f, 0.0f };
    _triangleMesh._vertices[1].color = { 0.0f, 1.0f, 0.0f };
    _triangleMesh._vertices[2].color = { 0.0f, 1.0f, 0.0f };
    
    upload_mesh(_triangleMesh);
}

void VulkanEngine::upload_mesh(Mesh& mesh)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = mesh._vertices.size() * sizeof(Vertex);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    
    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &mesh._vertexBuffer._buffer, &mesh._vertexBuffer._allocation, nullptr));
    
    _mainDeletionQueue.push_function([=]() {
        vmaDestroyBuffer(_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation);
    });
    
    void* data;
    vmaMapMemory(_allocator, mesh._vertexBuffer._allocation, &data);
    memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(_allocator, mesh._vertexBuffer._allocation);
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU, _device, _surface };
        
    vkb::Swapchain vkbSwapchain = swapchainBuilder
        .use_default_format_selection()
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(_windowExtent.width, _windowExtent.height)
        .build()
        .value();
    
    _swapchainExtent = vkbSwapchain.extent;
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
    
    _swapchainImageFormat = vkbSwapchain.image_format;
    
    _mainDeletionQueue.push_function([=]() {
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    });
}

bool VulkanEngine::load_shader_module(const char* filePath, VkShaderModule* outShaderModule)
{
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        return false;
    }
    
    size_t fileSize = static_cast<size_t>(file.tellg());
    
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    
    file.seekg(0);
    
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    
    file.close();
    
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;
    
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }
    *outShaderModule = shaderModule;
    return true;
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass)
{
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;
    
    viewportState.viewportCount = 1;
    viewportState.pViewports = &_viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &_scissor;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;
    
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &_colorBlendAttachment;
    
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    
    pipelineInfo.stageCount = static_cast<uint32_t>(_shaderStages.size());
    pipelineInfo.pStages = _shaderStages.data();
    pipelineInfo.pVertexInputState = &_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState = &_multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.renderPass = pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
    {
        std::cerr << "Failed to create pipeline" << std::endl;
        return VK_NULL_HANDLE;
    }
    return newPipeline;
}
