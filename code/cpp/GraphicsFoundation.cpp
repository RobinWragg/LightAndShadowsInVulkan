#include "GraphicsFoundation.h"

GraphicsFoundation::GraphicsFoundation(SDL_Window *window) {
  
  gfx::createCoreHandles(window);
}

void GraphicsFoundation::setMemory(VkDeviceMemory memory, uint64_t dataSize, const void *data) const {
  
  uint8_t *mappedMemory;
  auto result = vkMapMemory(gfx::device, memory, 0, dataSize, 0, (void**)&mappedMemory);
  SDL_assert(result == VK_SUCCESS);
      
  memcpy(mappedMemory, data, dataSize);
  vkUnmapMemory(gfx::device, memory);
}

VkExtent2D GraphicsFoundation::getSurfaceExtent() const {
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gfx::physDevice, gfx::surface, &capabilities);
  return capabilities.currentExtent;
}














