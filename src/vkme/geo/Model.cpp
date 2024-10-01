
#include <vkme/geo/Model.hpp>

#include "stb_image.h"
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <glm/glm.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

#include <tiny_obj_loader.h>

#include <fstream>


namespace vkme {
namespace geo {

std::vector<std::shared_ptr<Model>> Model::loadGltf(VulkanData* vulkanData, const std::filesystem::path& filePath, bool overrideColors)
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
        if (overrideColors) {
            for (Vertex& vtx : vertices) {
                vtx.setColor(glm::vec4(vtx.normal(), 1.f));
            }
        }
        newMesh._meshBuffers = std::unique_ptr<MeshBuffers>(MeshBuffers::uploadMesh(vulkanData, indices, vertices));

        meshes.emplace_back(std::make_shared<Model>(std::move(newMesh)));
    }

    return meshes;
}

std::vector<std::shared_ptr<Model>> Model::loadObj(
    VulkanData* vulkanData,
    const std::filesystem::path& filePath,
    const std::vector<std::shared_ptr<Modifier>>& modifiers
) {
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        throw std::runtime_error(std::string("Could not open OBJ model file: ") + filePath.string());
    }
    
    return Model::loadObj(vulkanData, file, filePath.filename().string(), modifiers);
}

std::vector<std::shared_ptr<Model>> Model::loadObj(
    VulkanData* vulkanData,
    std::istream& inputStream,
    const std::string& name,
    const std::vector<std::shared_ptr<Modifier>>& modifiers
) {
    std::vector<std::shared_ptr<Model>> result;
    
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    
    std::string warn;
    std::string err;

    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &inputStream);
    if (!warn.empty())
    {
        std::cout << "WARN: " << warn << std::endl;
    }
    
    if (!err.empty())
    {
        throw std::runtime_error(std::string("Error loading obj file: ") + err);
    }
    
    for (size_t s = 0; s < shapes.size(); ++s)
    {
		size_t index_offset = 0;
        std::vector<Vertex> vertexBufferData;
        std::vector<uint32_t> indices;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            //hardcode loading to triangles
			int fv = 3;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                // vertex position
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                // vertex normal
            	tinyobj::real_t nx = attrib.normals[3 * idx.normal_index];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                // vertex uv
                tinyobj::real_t s = attrib.texcoords[2 * idx.texcoord_index];
                tinyobj::real_t t = attrib.texcoords[2 * idx.texcoord_index + 1];

                //copy it into our vertex
				Vertex newVert(
                    { vx, vy, vz },
                    { nx, ny, nz },
                    { s, 1.0 - t },
                    { 1.0f, 1.0f, 1.0f, 1.0f }
                );

                vertexBufferData.push_back(newVert);
                indices.push_back(uint32_t(indices.size()));
			}
			index_offset += fv;
		}
        GeoSurface surface = {};
        surface.startIndex = 0;
        surface.indexCount = uint32_t(indices.size());
        for (auto& mod : modifiers)
        {
            mod->apply(indices, vertexBufferData);
        }
        MeshBuffers * meshBuffer = MeshBuffers::uploadMesh(vulkanData, indices, vertexBufferData);
        result.push_back(std::shared_ptr<Model>(
            new Model(name, { surface }, meshBuffer)
        ));
	}
    
    return result;
}

void Model::cleanup()
{
    _meshBuffers->cleanup();
}

void Model::allocateMaterialDescriptorSets(core::DescriptorSetAllocator* allocator, VkDescriptorSetLayout descriptorLayout)
{
    _useMaterialDescriptorSets = true;
    _materialDescriptorSets.resize(numSurfaces());
    for (auto& ds : _materialDescriptorSets)
    {
        ds = std::unique_ptr<core::DescriptorSet>(allocator->allocate(descriptorLayout));
    }
}

void Model::updateDescriptorSets(std::function<void(core::DescriptorSet*)>&& updateFunc)
{
    if (!_useMaterialDescriptorSets)
    {
        return;
    }

    for (uint32_t index = 0; index < numSurfaces(); ++index)
    {
        updateFunc(_materialDescriptorSets[index].get());
    }
}

void Model::draw(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, core::DescriptorSet* descriptorSets[], uint32_t numDescriptorSets)
{
    vkme::geo::MeshPushConstants pushConstants;
    pushConstants.modelMatrix = modelMatrix();

    // TODO: Do not use push constants
    //pushConstants.normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix())));
    
    pushConstants.vertexBufferAddress = meshBuffers()->vertexBufferAddress;
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vkme::geo::MeshPushConstants), &pushConstants);
    vkCmdBindIndexBuffer(cmd, meshBuffers()->indexBuffer->buffer(), 0, VK_INDEX_TYPE_UINT32);
    
    auto i = 0;
    for (auto s : surfaces())
    {
        uint32_t descriptorSetCount = _useMaterialDescriptorSets ? 1 + numDescriptorSets : numDescriptorSets;
        if (descriptorSetCount > 0)
        {
            std::vector<VkDescriptorSet> sets;
            sets.resize(descriptorSetCount);
            for (uint32_t j = 0; j < numDescriptorSets; ++j)
            {
                sets[j] = descriptorSets[j]->descriptorSet();
            }

            if (_useMaterialDescriptorSets)
            {
                sets[numDescriptorSets] = _materialDescriptorSets[i]->descriptorSet();
            }

            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout, 0,
                descriptorSetCount,
                sets.data(),
                0, nullptr
            );
        }
        
        vkCmdDrawIndexed(cmd, s.indexCount, 1, s.startIndex, 0, 0);
        ++i;
    }
}

}
}
