# mini engine

## Setup macOS

We will use Xcode, and because Apple's desktop applications are not able to read user-defined environment variables, as these are defined by the terminal's env files, there are some additional steps to set up the project.

The project is designed to be used with VulkanSDK version 1.3.290.0, **installed in the default location** ($(HOME)/VulkanSDK/1.3.290.0), and **(VERY IMPORTANT) also installing the GLM, VMA and SDL2 bible headers**.

If you have VulkanSDK installed in another location, you will need to modify the VulkanSDK location configuration setting. In the project properties, select Build Settings. Make sure you show all the project properties, and look for VULKAN_SDK_PATH. In the `User-Defined` section you should see a property with this name. Type in the location where you have VulkanSDK installed. The default value for this property is:

`$(HOME)/VulkanSDK/1.3.290.0/macOS`

In addition to this, you may well find that the `vulkan.framework` and `libSDL2-2.0.0.dylib` files appear red in the project navigator. This is because the Vulkan SDK is installed in the user folder, and there is no way to specify the path to these files in Xcode based on the current user folder. You have to select each file, and in the properties panel on the right, specify the specific location within the VulkanSDK, and then browse for them again with the folder button:

![code-framework-search](doc/xcode-framework-search.png)

- vulkan.framework: `~/VulkanSDK/1.3.290.0/macOS/Frameworks/vulkan.framework`
- libSDL2-2.0.0.dylib: `~/VulkanSDK/1.3.290.0/macOS/lib/libSDL2-2.0.0.dylib`

The rest of the dependencies are included at the project