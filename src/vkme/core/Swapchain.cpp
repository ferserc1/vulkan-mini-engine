#include <vkme/core/Swapchain.hpp>

#include <vkme/VulkanData.hpp>

namespace vkme {
namespace core {

void Swapchain::init(VulkanData * vulkanData, uint32_t width, uint32_t height)
{
	_vulkanData = vulkanData;

	vkb::SwapchainBuilder builder{
		vulkanData->physicalDevice(),
		vulkanData->device(),
		vulkanData->surface()
	};

	// TODO: IMGUI have an that prevents to use the specified image format 
	// if dynamic rendering is used. It is necesary to use this format to draw the UI
	// prevent this error
	//_imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    _imageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	VkSurfaceFormatKHR desiredFormat = {};
	desiredFormat.format = _imageFormat;
	desiredFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	vkb::Swapchain vkbSwapchain = builder
		.set_desired_format(desiredFormat)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	_extent = vkbSwapchain.extent;
	_swapchain = vkbSwapchain.swapchain;
	_images = vkbSwapchain.get_images().value();
	_imageViews = vkbSwapchain.get_image_views().value();
 
    _depthImage = Image::createAllocatedImage(
        vulkanData,
        VK_FORMAT_D32_SFLOAT,
        _extent,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
    
    // Array of core::Image wrappers
    _colorImages.clear();
    for (auto i = 0; i < _images.size(); ++i)
    {
        _colorImages.push_back(Image::wrapSwapchainImage(this, i));
    }
}

void Swapchain::resize(uint32_t width, uint32_t height)
{
    cleanup();
    
    vkb::SwapchainBuilder builder{
		_vulkanData->physicalDevice(),
		_vulkanData->device(),
		_vulkanData->surface()
	};
 
    _imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    
    VkSurfaceFormatKHR desiredFormat = {};
	desiredFormat.format = _imageFormat;
	desiredFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	vkb::Swapchain vkbSwapchain = builder
		.set_desired_format(desiredFormat)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	_extent = vkbSwapchain.extent;
	_swapchain = vkbSwapchain.swapchain;
	_images = vkbSwapchain.get_images().value();
	_imageViews = vkbSwapchain.get_image_views().value();
 
    _depthImage = Image::createAllocatedImage(
        _vulkanData,
        VK_FORMAT_D32_SFLOAT,
        _extent,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
    
    // Array of core::Image wrappers
    _colorImages.clear();
    for (auto i = 0; i < _images.size(); ++i)
    {
        _colorImages.push_back(Image::wrapSwapchainImage(this, i));
    }
}

void Swapchain::cleanup()
{
    if (_vulkanData)
    {
        // This images should not be cleared because they are wrappers
        // of the swapchain images and image views, that are cleared
        // later in this function
        _colorImages.clear();
        
        _depthImage->cleanup();
        delete _depthImage;
        
        destroySwapchain(_vulkanData->device(), _swapchain, nullptr);

        for (auto it = _imageViews.begin(); it != _imageViews.end(); ++it)
        {
            vkDestroyImageView(_vulkanData->device(), *it, nullptr);
        }
    }
}

}
}

