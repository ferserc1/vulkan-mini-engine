#pragma once

#include <vkme/core/common.hpp>
#include <vkme/geo/mesh_data.hpp>
#include <vkme/VulkanData.hpp>

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
    inline const GeoSurface& surface(uint32_t index) const { return _surfaces[index]; }

protected:
    std::string _name;

    std::vector<GeoSurface> _surfaces;
    std::unique_ptr<MeshBuffers> _meshBuffers;
};

}
}
