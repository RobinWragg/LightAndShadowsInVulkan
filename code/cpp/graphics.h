#pragma once

#include "main.h"
#include <vulkan/vulkan.h>

namespace gfx {
  
  const VkFormat surfaceFormat            = VK_FORMAT_B8G8R8A8_UNORM;
  const VkColorSpaceKHR surfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  
  const vector<const char *> requiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };
  
  extern VkInstance               instance;
  extern VkDebugUtilsMessengerEXT debugMsgr;
  extern VkSurfaceKHR             surface;
  extern VkPhysicalDevice         physDevice;
  extern VkDevice                 device;
  extern VkQueue                  queue;
  extern int                      queueFamilyIndex;
  
  // creators
  void createCoreHandles(SDL_Window *window);
  void createBuffer(VkBufferUsageFlagBits usage, uint64_t dataSize, VkBuffer *bufferOut, VkDeviceMemory *memoryOut);
  
  // getters
  vector<const char*> getRequiredLayers();
  void getAvailableInstanceLayers(vector<VkLayerProperties> *layerProperties);
  VkPhysicalDevice getPhysicalDevice(SDL_Window *window);
  uint32_t getMemoryType(uint32_t memTypeBits, VkMemoryPropertyFlags properties);
}





