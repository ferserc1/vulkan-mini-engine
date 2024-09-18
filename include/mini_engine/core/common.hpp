#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-deprecated-sync"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wunreachable-code-fallthrough"

#include "vk_mem_alloc.h"

#pragma clang diagnostic pop

#include <VkBootstrap.h>
#include <iostream>
#include <stdexcept>

#define VK_ASSERT(x)                                                                         \
    do {                                                                                     \
        VkResult err = x;                                                                    \
        if (err) {                                                                           \
            std::string errorString = std::string("Vulkan error: ") + string_VkResult(err);  \
            std::cerr << errorString << std::endl;                                           \
            throw std::runtime_error(errorString);                                           \
        }                                                                                    \
    } while(0)
