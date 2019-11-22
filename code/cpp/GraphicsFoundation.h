#pragma once
#include "main.h"

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

class GraphicsFoundation {
  public:
    VkInstance instance                = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMsgr = VK_NULL_HANDLE;
    VkSurfaceKHR surface               = VK_NULL_HANDLE;
    VkPhysicalDevice physDevice        = VK_NULL_HANDLE;
    VkDevice device                    = VK_NULL_HANDLE;
    VkQueue graphicsQueue              = VK_NULL_HANDLE;
    VkQueue surfaceQueue               = VK_NULL_HANDLE;
    
    GraphicsFoundation(SDL_Window *window, PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);
    ~GraphicsFoundation();
    
  private:
    enum class QueueType;
    
    void printAvailableInstanceLayers();
    
    VkInstance createInstance(SDL_Window *window, const vector<const char*> &layers);
    
    VkDebugUtilsMessengerEXT createDebugMessenger(PFN_vkDebugUtilsMessengerCallbackEXT callback);
    
    VkPhysicalDevice createPhysicalDevice(
      SDL_Window *window,
      const vector<const char *> &deviceExtensions,
      VkFormat requiredFormat,
      VkColorSpaceKHR requiredColorSpace);
    
    VkDeviceQueueCreateInfo createQueueInfo(QueueType queueType);
    
    void createDeviceAndQueues(
      const vector<const char*> &layers,
      const vector<const char *> &deviceExtensions);
};









