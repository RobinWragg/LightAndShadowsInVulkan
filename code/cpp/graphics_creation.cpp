#include "graphics_creation.h"

VkSwapchainKHR createSwapchain(const GraphicsFoundation *foundation) {
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    foundation->physDevice, foundation->surface, &capabilities);

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    foundation->physDevice, foundation->surface, &presentModeCount, nullptr);
  
  vector<VkPresentModeKHR> presentModes(presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    foundation->physDevice, foundation->surface, &presentModeCount, presentModes.data());

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = foundation->surface;
  createInfo.minImageCount = 2; // Double buffered
  createInfo.imageFormat = foundation->format;
  createInfo.imageColorSpace = foundation->colorSpace;
  createInfo.imageExtent = capabilities.currentExtent;
  createInfo.imageArrayLayers = 1; // 1 == not stereoscopic
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Suitable for VkFrameBuffer
  
  // If the graphics and surface queues are the same, no sharing is necessary.
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  
  createInfo.preTransform = capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Opaque window
  createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // vsync
  
  // Vulkan may not renderall pixels if some are osbscured by other windows.
  createInfo.clipped = VK_TRUE;
  
  // I don't currently support swapchain recreation.
  createInfo.oldSwapchain = VK_NULL_HANDLE;
  
  VkSwapchainKHR swapchain;
  auto result = vkCreateSwapchainKHR(foundation->device, &createInfo, nullptr, &swapchain);
  SDL_assert_release(result == VK_SUCCESS);
  
  return swapchain;
}











