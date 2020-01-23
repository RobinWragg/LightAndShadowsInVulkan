#pragma once

#include "main.h"
#include <vulkan/vulkan.h>

namespace gfx {
  
  extern const VkFormat surfaceFormat;
  extern const VkColorSpaceKHR surfaceColorSpace;
  
  extern const vector<const char *> requiredDeviceExtensions;
  
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
  VkExtent2D getSurfaceExtent();
  VkPhysicalDevice getPhysicalDevice(SDL_Window *window);
  uint32_t getMemoryType(uint32_t memTypeBits, VkMemoryPropertyFlags properties);
  
  // miscellaneous
  void setMemory(VkDeviceMemory memory, uint64_t dataSize, const void *data);
}





