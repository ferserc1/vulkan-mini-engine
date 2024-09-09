// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.
#pragma once

#include <platform_utils.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

struct AllocatedBuffer {
    VkBuffer _buffer;
    VmaAllocation _allocation;
};

//#include <fmt/core.h>
#include <iostream>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            std::cerr << "Detected Vulkan error:" << string_VkResult(err) << std::endl; \
            abort();                                                    \
        }                                                               \
    } while (0)
