
#pragma once

#include <vkme/geo/Model.hpp>

namespace vkme {
namespace geo {

class Sphere : public Model
{
public:
    static std::shared_ptr<Model> createUvSphere(VulkanData* vulkanData, float radius, const std::string& name = "sphere", const std::vector<std::shared_ptr<Modifier>>& mods = {});
    
};


}
}
