#pragma once

#include <vkme/core/common.hpp>
#include <vkme/core/DescriptorSet.hpp>
#include <vkme/geo/mesh_data.hpp>
#include <vkme/VulkanData.hpp>
#include <vkme/core/DescriptorSetAllocator.hpp>

#include <vector>
#include <filesystem>


namespace vkme {
namespace geo {

class Model
{
public:
    struct GeoSurface
    {
        uint32_t startIndex;
        uint32_t indexCount;
    };

    static std::vector<std::shared_ptr<Model>> loadGltf(VulkanData* vulkanData, const std::filesystem::path& filePath);

    void cleanup();

    inline const MeshBuffers* meshBuffers() const { return _meshBuffers.get(); }
    inline const uint32_t numSurfaces() const { return uint32_t(_surfaces.size()); }
    inline const const std::vector<GeoSurface>& surfaces() const { return _surfaces; }
    inline const GeoSurface& surface(uint32_t index) { return _surfaces[index]; }
    inline const glm::mat4& modelMatrix() const { return _modelMatrix; }
    inline glm::mat4& modelMatrix() { return _modelMatrix; }
    inline void setModelMatrix(const glm::mat4& modelMatrix) { _modelMatrix = modelMatrix; }

    void allocateMaterialDescriptorSets(core::DescriptorSetAllocator* allocator, VkDescriptorSetLayout descriptorLayout);
    void updateDescriptorSets(std::function<void(core::DescriptorSet*)>&& updateFunc);

    void draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, std::function<void(core::DescriptorSet*)>&& updateDsFunc);

protected:
    std::string _name;

    std::vector<GeoSurface> _surfaces;
    std::vector<std::unique_ptr<core::DescriptorSet>> _materialDescriptorSets;
    std::unique_ptr<MeshBuffers> _meshBuffers;
    glm::mat4 _modelMatrix;
};

}
}
