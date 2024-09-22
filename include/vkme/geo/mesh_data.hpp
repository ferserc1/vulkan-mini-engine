#pragma once

#include <vkme/core/common.hpp>
#include <vkme/core/Buffer.hpp>


namespace vkme {
namespace geo {

class Vertex
{
public:
    Vertex()
        : _position(glm::vec3(0.0f)), _normal(glm::vec3(0.0f)), _uvX(0), _uvY(0), _color(glm::vec4(0.0f)) {}
    Vertex(const glm::vec3 & position, const glm::vec3 & normal, const glm::vec2& uv1, const glm::vec4& color = glm::vec4(1.0f))
        : _position(position), _normal(normal), _uvX(uv1.x), _uvY(uv1.y), _color(color) {}
    ~Vertex() {}

    // Getters and setters for the vertex attributes
    inline const glm::vec3& position() const { return _position; }
    inline void setPosition(const glm::vec3 & position) { this->_position = position; }

    inline const glm::vec3& normal() const { return _normal; }
    inline void setNormal(const glm::vec3 & normal) { this->_normal = normal; }

    inline const glm::vec2 uv1() const { return glm::vec2(_uvX, _uvY); }
    inline void setUv1(const glm::vec2 & uv1) { this->_uvX = uv1.x; this->_uvY = uv1.y; }

    inline const glm::vec4& color() const { return _color; }
    inline void setColor(const glm::vec4 & color) { this->_color = color; }

protected:
    glm::vec3 _position;
    float _uvX;
    glm::vec3 _normal;
    float _uvY;
    glm::vec4 _color;

    // TODO: tangent uv2X uv2Y
};

class MeshBuffers
{
public:
    core::Buffer* indexBuffer = nullptr;
    core::Buffer* vertexBuffer = nullptr;
    VkDeviceAddress vertexBufferAddress = 0;
    uint32_t indexCount = 0;

    static MeshBuffers* uploadMesh(VulkanData* vulkanData, const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices);

    void cleanup();

};

struct MeshPushConstants
{
    glm::mat4 modelMatrix;
    VkDeviceAddress vertexBufferAddress;
};

}
}
