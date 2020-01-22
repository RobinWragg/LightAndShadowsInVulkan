#pragma once

#include "main.h"
#include <vulkan/vulkan.h>

namespace gfx {
  
  const VkFormat surfaceFormat            = VK_FORMAT_B8G8R8A8_UNORM;
  const VkColorSpaceKHR surfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  
  const vector<const char *> requiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };
  
  // creators
  VkInstance createInstance(SDL_Window *window, VkDebugUtilsMessengerEXT *optionalDebugMsgrOut);
  void createDeviceAndQueue(VkSurfaceKHR surface, VkPhysicalDevice physDevice, VkDevice *deviceOut, VkQueue *queueOut, int *queueFamilyIndexOut);
  
  // getters
  vector<const char*> getRequiredLayers();
  void getAvailableInstanceLayers(vector<VkLayerProperties> *layerProperties);
  VkPhysicalDevice getPhysicalDevice(SDL_Window *window, VkInstance instance, VkSurfaceKHR surface);
}





