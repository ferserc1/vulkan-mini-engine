#pragma once

#ifndef _WIN32
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-deprecated-sync"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wunreachable-code-fallthrough"
#endif

#include "vk_mem_alloc.h"

#ifndef _WIN32
#pragma clang diagnostic pop
#endif 

namespace vkme {
namespace core {

void* getMappedData(VmaAllocation a);


}
}

