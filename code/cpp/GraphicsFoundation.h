#pragma once
#include "graphics.h"

class GraphicsFoundation {
  public:
    GraphicsFoundation(SDL_Window *window);
    
    VkExtent2D getSurfaceExtent() const;
    
    void setMemory(VkDeviceMemory memory, uint64_t dataSize, const void *data) const;
    
  private:
    VkDebugUtilsMessengerEXT createDebugMessenger();
};









