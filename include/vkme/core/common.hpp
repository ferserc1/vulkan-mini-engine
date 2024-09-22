#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <vkme/core/vma_allocation.hpp>

#include <VkBootstrap.h>
#include <iostream>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vkme/core/extensions.hpp>

#define VK_ASSERT(x)                                                                         \
    do {                                                                                     \
        VkResult err = x;                                                                    \
        if (err) {                                                                           \
            std::string errorString = std::string("Vulkan error: ") + string_VkResult(err);  \
            std::cerr << errorString << std::endl;                                           \
            throw std::runtime_error(errorString);                                           \
        }                                                                                    \
    } while(0)
