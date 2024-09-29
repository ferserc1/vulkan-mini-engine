#include <vkme/geo/Modifiers.hpp>

namespace vkme {
namespace geo {

void FlipFacesModifier::apply(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices)
{
    for (auto i = 0; i < indices.size(); i += 3)
    {
        auto a = indices[i];
        auto b = indices[i + 2];
        indices[i] = b;
        indices[i + 2] = a;
    }
}

void FlipNormalsModifier::apply(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices)
{
    for (auto& v : vertices)
    {
        auto n = v.normal();
        v.setNormal(n * -1.0f);
    }
}

void ScaleModifier::apply(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices)
{
    for (auto& v : vertices)
    {
        auto pos = v.position();
        v.setPosition({ pos.x * _sx, pos.y * _sy, pos.z * _sz });
    }
}

void OverrideColorsModifier::apply(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices)
{
    for (auto& v : vertices)
    {
        auto n = v.normal();
        v.setColor({ n.x, n.y, n.z, 1.0f });
    }
}
    
}
}
