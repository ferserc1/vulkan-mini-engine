
#include <vkme/geo/Cube.hpp>
#include <sstream>

namespace vkme::geo {

const char * cube_objData = R""""(
o Cube
v 0.5000000 0.5000000 -0.5000000
v 0.5000000 -0.5000000 -0.5000000
v 0.5000000 0.5000000 0.5000000
v 0.5000000 -0.5000000 0.5000000
v -0.5000000 0.5000000 -0.5000000
v -0.5000000 -0.5000000 -0.5000000
v -0.5000000 0.5000000 0.5000000
v -0.5000000 -0.5000000 0.5000000
vt 0.007582 0.995648
vt 0.007582 0.005685
vt 0.992623 0.005685
vt 0.992623 0.995648
vt 0.992623 0.995648
vt 0.007582 0.995648
vt 0.007582 0.005685
vt 0.992623 0.005685
vt 0.992623 0.995648
vt 0.007582 0.995648
vt 0.007582 0.005685
vt 0.007582 0.995648
vt 0.007582 0.005685
vt 0.992623 0.005685
vt 0.992623 0.995648
vt 0.992623 0.995648
vt 0.007582 0.005685
vt 0.992623 0.005685
vt 0.992623 0.995648
vn 0.0000 1.0000 0.0000
vn 0.0000 0.0000 1.0000
vn -1.0000 0.0000 0.0000
vn 0.0000 -1.0000 0.0000
vn 1.0000 0.0000 0.0000
vn 0.0000 0.0000 -1.0000
usemtl Cube
s off
f 1/1/1 5/2/1 7/3/1 3/4/1
f 4/5/2 3/6/2 7/7/2 8/8/2
f 8/8/3 7/9/3 5/10/3 6/11/3
f 6/12/4 2/13/4 4/14/4 8/15/4
f 2/16/5 1/1/5 3/17/5 4/14/5
f 6/18/6 5/19/6 1/1/6 2/13/6
)"""";

std::shared_ptr<Model> Cube::createCube(VulkanData* vulkanData, float side, const std::string& name, const std::vector<std::shared_ptr<Modifier>>& mods)
{
    std::stringstream objStream(cube_objData);
    
    std::vector<std::shared_ptr<Modifier>> modifiers(mods.begin(), mods.end());
    if (side != 1.0f)
    {
        modifiers.push_back(std::unique_ptr<Modifier>(new ScaleModifier(side)));
    }
    
    auto result = Model::loadObj(vulkanData, objStream, name, modifiers)[0];
    
    return result;
}

}
