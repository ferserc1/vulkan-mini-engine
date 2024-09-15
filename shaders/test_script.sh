#!/bin/sh

VULKAN_SDK=~/VulkanSDK/1.3.290.0/macOS \
PROJECT_DIR=. \
BUILT_PRODUCTS_DIR=. \
UNLOCALIZED_RESOURCES_FOLDER_PATH=compiled \
./build_shaders_macos.sh
