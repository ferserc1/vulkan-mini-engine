#pragma once

#include <SDL.h>
#include <SDL_vulkan.h>

#include <string>
#include <memory>

#include <vkme/VulkanData.hpp>
#include <vkme/DrawLoop.hpp>
#include <vkme/UserInterface.hpp>

namespace vkme {
    
class MainLoop {
public:
    
    inline void initWindowSize(uint32_t width, uint32_t height) { _windowWidth = width; _windowHeight = height; }
    inline void initWindowTitle(const std::string& title) { _windowTitle = title; }
    inline void setDrawLoopDelegate(std::shared_ptr<DrawLoopDelegate> d) { _drawLoop.setDelegate(d); }
    inline void setUIDelegate(std::shared_ptr<UserInterfaceDelegate> d) { _userInterface.setDelegate(d); }
    int32_t run();

protected:
    uint32_t _windowWidth = 1440;
    uint32_t _windowHeight = 700;
    std::string _windowTitle = "Vulkan Mini Engine";

    VulkanData _vulkanData;
    DrawLoop _drawLoop;
    UserInterface _userInterface;
};

}
