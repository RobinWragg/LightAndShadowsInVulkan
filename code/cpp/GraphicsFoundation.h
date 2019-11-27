#pragma once
#include "main.h"

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

class GraphicsFoundation {
  public:
    GraphicsFoundation(SDL_Window *window, PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);
    ~GraphicsFoundation();
    
    VkInstance instance                = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMsgr = VK_NULL_HANDLE;
    VkSurfaceKHR surface               = VK_NULL_HANDLE;
    VkPhysicalDevice physDevice        = VK_NULL_HANDLE;
    VkDevice device                    = VK_NULL_HANDLE;
    VkQueue graphicsQueue              = VK_NULL_HANDLE;
    VkQueue surfaceQueue               = VK_NULL_HANDLE;
    
    const VkFormat format            = VK_FORMAT_B8G8R8A8_UNORM;
    const VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    
    const vector<const char*> layers = {
#ifdef __APPLE__
      "MoltenVK"
#else
      "VK_LAYER_LUNARG_api_dump",
      "VK_LAYER_LUNARG_standard_validation",
      "VK_LAYER_KHRONOS_validation"
#endif
    };
    
    const vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
  private:
    enum class QueueType;
    
    void printAvailableInstanceLayers();
    
    VkInstance createInstance(SDL_Window *window);
    
    VkDebugUtilsMessengerEXT createDebugMessenger(PFN_vkDebugUtilsMessengerCallbackEXT callback);
    
    VkPhysicalDevice createPhysicalDevice(SDL_Window *window);
    
    VkDeviceQueueCreateInfo createQueueInfo(QueueType queueType);
    
    void createDeviceAndQueues();
};









