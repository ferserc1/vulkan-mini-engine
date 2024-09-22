
#include <vkme/geo/mesh_data.hpp>
#include <vkme/core/Buffer.hpp>

#include "vk_mem_alloc.h"

namespace vkme {
namespace geo {

MeshBuffers* MeshBuffers::uploadMesh(
    VulkanData* vulkanData,
    const std::vector<uint32_t>& indices,
    const std::vector<Vertex>& vertices
) {
    auto meshBuffers = new MeshBuffers();
    
    auto vertexBufferSize = vertices.size() * sizeof(Vertex);
    auto indexBufferSize = indices.size() * sizeof(uint32_t);
    
    meshBuffers->vertexBuffer = core::Buffer::createAllocatedBuffer(
        vulkanData,
        vertexBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );
    
    meshBuffers->vertexBufferAddress = meshBuffers->vertexBuffer->deviceAddress();
    
    meshBuffers->indexBuffer = core::Buffer::createAllocatedBuffer(
        vulkanData,
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    meshBuffers->indexCount = uint32_t(indices.size());
    
    auto stagingBuffer = core::Buffer::createAllocatedBuffer(
        vulkanData,
        vertexBufferSize + indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    );
    
    void* data = stagingBuffer->allocatedData();
    
    memcpy(data, vertices.data(), vertexBufferSize);
    memcpy(reinterpret_cast<char*>(data) + vertexBufferSize, indices.data(), indexBufferSize);
    
    vulkanData->command().immediateSubmit([&](VkCommandBuffer cmd) {
        VkBufferCopy vertexCopy = {};
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;
        vkCmdCopyBuffer(cmd, stagingBuffer->buffer(), meshBuffers->vertexBuffer->buffer(), 1, &vertexCopy);
        
        VkBufferCopy indexCopy = {};
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.size = indexBufferSize;
        vkCmdCopyBuffer(cmd, stagingBuffer->buffer(), meshBuffers->indexBuffer->buffer(), 1, &indexCopy);
    });

    stagingBuffer->cleanup();
    
    return meshBuffers;
}

void MeshBuffers::cleanup()
{
    if (indexBuffer != nullptr)
    {
        indexBuffer->cleanup();
        vertexBuffer->cleanup();
        vertexBufferAddress = 0;
    }
    
}
    
}
}
