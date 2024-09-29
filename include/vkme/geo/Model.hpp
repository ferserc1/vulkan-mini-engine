#pragma once

#include <vkme/core/common.hpp>
#include <vkme/core/DescriptorSet.hpp>
#include <vkme/geo/mesh_data.hpp>
#include <vkme/VulkanData.hpp>
#include <vkme/core/DescriptorSetAllocator.hpp>
#include <vkme/geo/Modifiers.hpp>

#include <vector>
#include <filesystem>
#include <istream>


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

    Model() = default;
    Model(const std::string& name, std::vector<GeoSurface>&& surfaces, MeshBuffers* meshBuffers)
        :_name{ name }, _surfaces{ std::move(surfaces) }, _meshBuffers{ meshBuffers }
    {}

    static std::vector<std::shared_ptr<Model>> loadGltf(VulkanData* vulkanData, const std::filesystem::path& filePath, bool overrideColors = false);
    static std::vector<std::shared_ptr<Model>> loadObj(
        VulkanData* vulkanData,
        const std::filesystem::path& filePath,
        const std::vector<std::shared_ptr<Modifier>>& modifiers = {}
    );
    static std::vector<std::shared_ptr<Model>> loadObj(
        VulkanData* vulkanData,
        std::istream& inputStream,
        const std::string& name = "obj model",
        const std::vector<std::shared_ptr<Modifier>>& modifiers = {}
    );

    void cleanup();

    inline const MeshBuffers* meshBuffers() const { return _meshBuffers.get(); }
    inline const uint32_t numSurfaces() const { return uint32_t(_surfaces.size()); }
    inline const std::vector<GeoSurface>& surfaces() const { return _surfaces; }
    inline std::vector<GeoSurface>& surfaces() { return _surfaces; }
    inline const GeoSurface& surface(uint32_t index) { return _surfaces[index]; }
    inline const glm::mat4& modelMatrix() const { return _modelMatrix; }
    inline glm::mat4& modelMatrix() { return _modelMatrix; }
    inline void setModelMatrix(const glm::mat4& modelMatrix) { _modelMatrix = modelMatrix; }

    void allocateMaterialDescriptorSets(core::DescriptorSetAllocator* allocator, VkDescriptorSetLayout descriptorLayout);
    void updateDescriptorSets(std::function<void(core::DescriptorSet*)>&& updateFunc);

    void draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, core::DescriptorSet* descriptorSets[] = nullptr, uint32_t numDescriptorSets = 0);

protected:
    std::string _name;

    std::vector<GeoSurface> _surfaces;
    std::vector<std::unique_ptr<core::DescriptorSet>> _materialDescriptorSets;
    bool _useMaterialDescriptorSets = false;
    std::unique_ptr<MeshBuffers> _meshBuffers;
    glm::mat4 _modelMatrix;
};

}
}
