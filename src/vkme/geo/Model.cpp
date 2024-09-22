
#include <vkme/geo/Model.hpp>

#include "stb_image.h"
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

namespace vkme {
namespace geo {

std::vector<std::shared_ptr<Model>> Model::loadGltf(VulkanData* vulkanData, const std::filesystem::path& filePath)
{
    std::cout << "Loading GLTF model file " << filePath << std::endl;
    
    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);
    
    constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers |
        fastgltf::Options::LoadExternalBuffers;
        
    fastgltf::Asset gltf;
    fastgltf::Parser parser;
    
    auto load = parser.loadBinaryGLTF(&data, filePath.parent_path(), gltfOptions);
    if (load)
    {
        gltf = std::move(load.get());
    }
    else {
        throw std::runtime_error(std::string("Error loading GLTF file at path: ") + filePath.string());
    }
    
    
    std::vector<std::shared_ptr<Model>> meshes;

    // use the same vectors for all meshes so that the memory doesnt reallocate as
    // often
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    for (fastgltf::Mesh& mesh : gltf.meshes) {
        Model newMesh;

        newMesh._name = mesh.name;

        // clear the mesh arrays each mesh, we dont want to merge them by error
        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives) {
            GeoSurface newSurface;
            newSurface.startIndex = (uint32_t)indices.size();
            newSurface.indexCount = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            size_t initialVertexIndex = vertices.size();

            // load indexes
            {
                fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
                    [&](std::uint32_t idx) {
                        indices.push_back(uint32_t(idx + initialVertexIndex));
                    });
            }

            // load vertex positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                    [&](glm::vec3 v, size_t index) {
                        Vertex vertex;
                        vertex.setPosition(v);
                        vertex.setNormal({ 1, 0, 0 });
                        vertex.setColor(glm::vec4 { 1.f });
                        vertex.setUv1({ 0, 0 });
                        vertices[initialVertexIndex + index] = vertex;
                    });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                    [&](glm::vec3 n, size_t index) {
                        vertices[initialVertexIndex + index].setNormal(n);
                    });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                    [&](glm::vec2 v, size_t index) {
                        vertices[initialVertexIndex + index].setUv1(v);
                    });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                    [&](glm::vec4 c, size_t index) {
                        vertices[initialVertexIndex + index].setColor(c);
                    });
            }
            newMesh._surfaces.push_back(newSurface);
        }

        // display the vertex normals
        constexpr bool OverrideColors = true;
        if (OverrideColors) {
            for (Vertex& vtx : vertices) {
                vtx.setColor(glm::vec4(vtx.normal(), 1.f));
            }
        }
        newMesh._meshBuffers = std::unique_ptr<MeshBuffers>(MeshBuffers::uploadMesh(vulkanData, indices, vertices));

        meshes.emplace_back(std::make_shared<Model>(std::move(newMesh)));
    }

    return meshes;
}

void Model::cleanup()
{
    _meshBuffers->cleanup();
}

}
}
