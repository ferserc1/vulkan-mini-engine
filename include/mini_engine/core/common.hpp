#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <VkBootstrap.h>
#include <iostream>

#define VK_ASSERT(x)                                                                \
    do {                                                                            \
        VkResult err = x;                                                           \
        if (err) {                                                                  \
            std::cerr << "Vulkan error: " << string_VkResult(err) << std::endl;     \
        }                                                                           \
    } while(0)
