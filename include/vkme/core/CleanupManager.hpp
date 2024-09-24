#pragma once

#include <deque>
#include <functional>
#include <vulkan/vulkan.h>

namespace vkme {
namespace core {

class CleanupManager {
public:
    
    void push(std::function<void(VkDevice)>&& fn)
    {
        _deleters.push_back(fn);
    }
    
    void flush(VkDevice device)
    {
        for (auto it = _deleters.rbegin(); it != _deleters.rend(); ++it)
        {
            (*it)(device);
        }
        _deleters.clear();
    }

protected:
    std::deque<std::function<void(VkDevice)>> _deleters;
};

}
}
