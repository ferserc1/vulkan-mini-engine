
#include <vkme/PlatformTools.hpp>

#ifdef MINI_ENGINE_IS_MAC

#include <CoreFoundation/CoreFoundation.h>

std::string mini_engine_platform_tools_macos_bundle_path()
{
    auto appBundle = CFBundleGetMainBundle();
    CFURLRef appUrlRef = CFBundleCopyBundleURL(appBundle);
    char c_path[2048] = {'\0'};
    
    CFURLGetFileSystemRepresentation(appUrlRef, true, reinterpret_cast<UInt8*>(c_path), 2048);
    
    CFRelease(appUrlRef);
    
    return std::string(c_path) + "/";
}

std::string mini_engine_platform_tools_macos_resources_path()
{
    auto appBundle = CFBundleGetMainBundle();
    auto resourcesUrl = CFBundleCopyResourcesDirectoryURL(appBundle);
    char c_path[2048] = {'\0'};
    
    CFURLGetFileSystemRepresentation(resourcesUrl, true, reinterpret_cast<UInt8*>(c_path), 2048);
    
    CFRelease(resourcesUrl);
    
    return std::string(c_path) + "/";
}

#endif

std::string vkme::PlatformTools::shaderPath()
{
#ifdef MINI_ENGINE_IS_MAC
    return mini_engine_platform_tools_macos_resources_path() + "shaders/";
#else
    return "shaders/";
#endif
}

std::string vkme::PlatformTools::assetPath()
{
#ifdef MINI_ENGINE_IS_MAC
    return mini_engine_platform_tools_macos_resources_path() + "assets/";
#else
    return "assets/";
#endif
}
