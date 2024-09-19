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
}

void Swapchain::cleanup()
{
	vkDestroySwapchainKHR(_vulkanData->device(), _swapchain, nullptr);

	for (auto it = _imageViews.begin(); it != _imageViews.end(); ++it)
	{
		vkDestroyImageView(_vulkanData->device(), *it, nullptr);
	}
}

}
}

