#pragma once

#include <deque>
#include <functional>

namespace miniengine {
namespace core {

class CleanupManager {
public:
    
    void push(std::function<void()>&& fn)
    {
        _deleters.push_back(fn);
    }
    
    void flush()
    {
        for (auto it = _deleters.rbegin(); it != _deleters.rend(); ++it)
        {
            (*it)();
        }
    }

protected:
    std::deque<std::function<void()>> _deleters;
};

}
}
