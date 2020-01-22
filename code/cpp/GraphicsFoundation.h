#pragma once
#include "graphics.h"

class GraphicsFoundation {
  public:
    GraphicsFoundation(SDL_Window *window);
    ~GraphicsFoundation();
    
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    
    VkExtent2D getSurfaceExtent() const;
    
    void createVkBuffer(VkBufferUsageFlagBits usage, uint64_t dataSize, VkBuffer *bufferOut, VkDeviceMemory *memoryOut) const;
    
    void setMemory(VkDeviceMemory memory, uint64_t dataSize, const void *data) const;
    
    VkInstance instance                = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMsgr = VK_NULL_HANDLE;
    VkSurfaceKHR surface               = VK_NULL_HANDLE;
    VkPhysicalDevice physDevice        = VK_NULL_HANDLE;
    VkDevice device                    = VK_NULL_HANDLE;
    VkQueue queue                      = VK_NULL_HANDLE;
    int queueFamilyIndex               = -1;
    
  private:
    VkDebugUtilsMessengerEXT createDebugMessenger();
};









