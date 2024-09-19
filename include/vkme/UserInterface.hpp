#pragma once

#include <vkme/core/common.hpp>
#include <vkme/VulkanData.hpp>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include <SDL.h>
#include <SDL_vulkan.h>


namespace vkme {

class UserInterface {
public:
    void init(VulkanData*);

    void processEvent(SDL_Event * event);

    void newFrame();
    
    void draw(VkCommandBuffer cmd, VkImageView targetImageView);

    void cleanup();
    
protected:
    VulkanData * _vulkanData;
    
    VkFence _uiFence;
    VkCommandBuffer _commandBuffer;
    VkCommandPool _commandPool;
    
    void initCommands();
    void initImGui();
};

}
