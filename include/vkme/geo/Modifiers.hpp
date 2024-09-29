#pragma once

#include <vkme/geo/mesh_data.hpp>

namespace vkme {
namespace geo {

class Modifier {
public:
    virtual ~Modifier() = default; 
    virtual void apply(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices) = 0;
};

class FlipFacesModifier : public Modifier {
public:
    void apply(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices);
};

class FlipNormalsModifier : public Modifier {
public:
    void apply(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices);
};

class ScaleModifier : public Modifier {
public:
    ScaleModifier(float scale) : _sx { scale }, _sy { scale }, _sz { scale } {}
    ScaleModifier(float sx, float sy, float sz) : _sx { sx }, _sy { sy }, _sz { sz } {}

    void apply(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices);
    
protected:
    float _sx, _sy, _sz;
};

class OverrideColorsModifier : public Modifier {
public:
    void apply(std::vector<uint32_t>& indices, std::vector<Vertex>& vertices);
};

}
}
