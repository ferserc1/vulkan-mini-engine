#pragma once

#include <string>

#ifdef __APPLE__

#define MINI_ENGINE_IS_MAC 1

static const bool is_mac = true;
static const bool is_windows = false;
static const bool is_linux = false;

#elif defined(_WIN32)

#define MINI_ENGINE_IS_WINDOWS 1

static const bool is_mac = false;
static const bool is_windows = true;
static const bool is_linux = false;

#else

#define MINI_ENGINE_IS_LINUX 1

static const bool is_mac = false;
static const bool is_windows = false;
static const bool is_linux = true;

#endif

namespace vkme {

class PlatformTools {
public:
    static std::string shaderPath();
    static std::string assetPath();
};

}
