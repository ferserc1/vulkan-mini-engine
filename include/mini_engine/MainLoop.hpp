#pragma once

#include <SDL.h>
#include <SDL_vulkan.h>

#include <string>

#include <mini_engine/VulkanData.hpp>
#include <mini_engine/DrawLoop.hpp>

namespace miniengine {
    
class MainLoop {
public:
    
    inline void initWindowSize(uint32_t width, uint32_t height) { _windowWidth = width; _windowHeight = height; }
    inline void initWindowTitle(const std::string& title) { _windowTitle = title; }
    inline void setDrawLoopDelegate(DrawLoopDelegate * d) { _drawLoop.setDrawDelegate(d); }
    int32_t run();

protected:
    uint32_t _windowWidth = 1440;
    uint32_t _windowHeight = 700;
    std::string _windowTitle = "Vulkan Mini Engine";

    VulkanData _vulkanData;
    DrawLoop _drawLoop;
};

}
