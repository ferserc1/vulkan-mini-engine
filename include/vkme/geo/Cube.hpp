
#pragma once

#include <vkme/geo/Model.hpp>

namespace vkme {
namespace geo {

class Cube : public Model
{
public:
    static std::shared_ptr<Model> createCube(VulkanData* vulkanData, float side, const std::string& name = "cube", const std::vector<std::shared_ptr<Modifier>>& mods = {});
    
};


}
}
