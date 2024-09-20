#pragma once

#include <vkme/core/common.hpp>
#include <vkme/VulkanData.hpp>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <memory>

namespace vkme {

class UserInterface;

class UserInterfaceDelegate {
public:
    virtual ~UserInterfaceDelegate() {}

    virtual void init(VulkanData*, UserInterface*) {}
    virtual void drawUI() = 0;
};

class UserInterface {
public:
    void init(VulkanData*);

    void processEvent(SDL_Event * event);

    void newFrame();
    
    void draw(VkCommandBuffer cmd, VkImageView targetImageView);

    void cleanup();

    inline void setDelegate(std::shared_ptr<UserInterfaceDelegate> delegate) { _delegate = delegate; }
    
protected:
    VulkanData * _vulkanData;
    
    VkFence _uiFence;
    VkCommandBuffer _commandBuffer;
    VkCommandPool _commandPool;
    VkDescriptorPool _imguiPool;

    std::shared_ptr<UserInterfaceDelegate> _delegate;
    
    void initCommands();
    void initImGui();
};

}
