#pragma once

#include <string>


#ifdef __APPLE__

static const bool is_mac = true;
static const bool is_windows = false;
static const bool is_linux = false;

#define MINI_ENGINE_IS_MAC 1

#elif defined(_WIN32)

static const bool is_mac = false;
static const bool is_windows = true;
static const bool is_linux = false;

#define MINI_ENGINE_IS_WINDOWS 1

#else

static const bool is_mac = false;
static const bool is_windows = false;
static const bool is_linux = true;

#define MINI_ENGINE_IS_LINUX 1

#endif

class PlatformUtils {
public:
    static std::string macos_get_bundle_path();
    static std::string macos_get_resources_path();
};

