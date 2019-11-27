#include "graphics_creation.h"

const int swapchainSize = 2; // Double buffered
vector<VkImage> swapchainImages;
vector<VkImageView> swapchainViews;

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
  createInfo.minImageCount = swapchainSize;
  createInfo.imageFormat = foundation->surfaceFormat;
  createInfo.imageColorSpace = foundation->surfaceColorSpace;
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

void createSwapchainImagesAndViews(VkSwapchainKHR swapchain, const GraphicsFoundation *foundation) {
  uint32_t imageCount;
  vkGetSwapchainImagesKHR(foundation->device, swapchain, &imageCount, nullptr);
  swapchainImages.resize(imageCount);
  swapchainViews.resize(imageCount);
  
  SDL_assert_release(imageCount == swapchainSize);
  
  vkGetSwapchainImagesKHR(foundation->device, swapchain, &imageCount, swapchainImages.data());

  for (int i = 0; i < swapchainImages.size(); i++) {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapchainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = foundation->surfaceFormat;
    
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    
    SDL_assert_release(vkCreateImageView(foundation->device, &createInfo, nullptr, &swapchainViews[i]) == VK_SUCCESS);
  }
}




















