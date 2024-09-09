
#include <platform_utils.h>

#ifdef MINI_ENGINE_IS_MAC

#include <CoreFoundation/CoreFoundation.h>

#endif

std::string PlatformUtils::macos_get_bundle_path()
{
#ifdef MINI_ENGINE_IS_MAC
    auto appBundle = CFBundleGetMainBundle();
    CFURLRef appUrlRef = CFBundleCopyBundleURL(appBundle);
    
    char c_path[2048] = {'\0'};
    
    CFURLGetFileSystemRepresentation(appUrlRef, true, reinterpret_cast<UInt8*>(c_path), 2048);
    
    CFRelease(appUrlRef);
    
    return std::string(c_path) + "/";
#else
    return "";
#endif
}

std::string PlatformUtils::macos_get_resources_path()
{
#ifdef MINI_ENGINE_IS_MAC
    auto appBundle = CFBundleGetMainBundle();
    auto resourcesUrl = CFBundleCopyResourcesDirectoryURL(appBundle);
    char c_path[2048] = {'\0'};
    CFURLGetFileSystemRepresentation(resourcesUrl, true, reinterpret_cast<UInt8*>(c_path), 2048);
    CFRelease(resourcesUrl);
    
    return std::string(c_path) + "/";
#else
    return "";
#endif
}
