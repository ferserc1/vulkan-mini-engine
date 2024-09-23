#pragma once

#include <vkme/DrawLoop.hpp>
#include <vkme/UserInterface.hpp>
#include <vkme/core/Buffer.hpp>


class VertexBuffersDelegate :
    // Required to draw
    public vkme::DrawLoopDelegate,
    
    // Required to use ImGui user interfaces. In this case
    // we aren't using UI, but in the main.cpp file we are
    // using the delegate as DrawLoopDelegate and
    // UserInterfaceDelegate
    public vkme::UserInterfaceDelegate
{
public:
    void init(vkme::VulkanData * vulkanData);
    
    // We don't need to do anything on swapchain resize, because we are
    // drawing directly on the swap chain image
    void swapchainResized(VkExtent2D) {}
    
    VkImageLayout draw(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const vkme::core::Image* colorImage,
        const vkme::core::Image* depthImage
    );
    
    // We don't need UI
    void drawUI() {}
    
    // Resources to create the vertex buffers
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec4 color;
        
        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription {};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }
        
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions {};
            
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].offset = offsetof(Vertex, color);
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            
            return attributeDescriptions;
        }
    };
    
    // Mesh data
    std::vector<Vertex> _vertices = {
        {{ 0.0f,  0.5f, 0.0f}, {1.0f, 0.5f, 0.5f, 1.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0.5f, 1.0f, 0.5f, 1.0f}},
        {{-0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 1.0f, 1.0f}}
    };

protected:
    vkme::VulkanData * _vulkanData;
    
    VkPipelineLayout _layout;
    VkPipeline _pipeline;
    
    // Buffer to store the vertex data
    vkme::core::Buffer * _vertexBuffer = nullptr;
    
    void initPipeline();
    
    void initMesh();
};
