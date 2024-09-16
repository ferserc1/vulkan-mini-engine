
#include <mini_engine/VulkanData.hpp>

namespace miniengine {

class DrawLoop {
public:
    inline void setVulkanData(VulkanData * vulkanData) { _vulkanData = vulkanData; }
    
    // TODO: Use a lambda function to record the command buffer
    void draw();

protected:
    VulkanData * _vulkanData = nullptr;
};

}
