#include "graphics.h"

namespace gfx {
  const vector<const char*> requiredDebugLayers = {
    #ifdef __APPLE__
      // macOS with validation (Vulkan SDK installation required)
      "VK_LAYER_LUNARG_standard_validation",
      "VK_LAYER_KHRONOS_validation"
    #else
      // Windows with validation
      "VK_LAYER_KHRONOS_validation"
    #endif
  };
  
  const vector<const char*> requiredReleaseLayers = {
    #ifdef __APPLE__
      // macOS without validation
      "MoltenVK"
    #else
      // Windows without validation
      // (no layers necessary)
    #endif
  };
  
  vector<const char*> getRequiredLayers() {
    #ifdef DEBUG
      return requiredDebugLayers;
    #else
      return requiredReleaseLayers;
    #endif
  }
  
  void getAvailableInstanceLayers(vector<VkLayerProperties> *layerProperties) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    layerProperties->resize(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties->data());
  }
  
  VkPhysicalDevice getPhysicalDevice(SDL_Window *window) {
    
    if (physDevice != VK_NULL_HANDLE) return physDevice;
    
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    vector<VkPhysicalDevice> availableDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, availableDevices.data());
    
    vector<VkPhysicalDevice> suitableDevices;
    
    for (auto &availableDevice : availableDevices) {
      
      // Require Vulkan 1.x
      {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(availableDevice, &properties);
        if (VK_VERSION_MAJOR(properties.apiVersion) != 1) continue;
      }
      
      // Check for required extensions
      {
        uint32_t count;
        vkEnumerateDeviceExtensionProperties(availableDevice, nullptr, &count, nullptr);
        vector<VkExtensionProperties> availableExtensions(count);
        vkEnumerateDeviceExtensionProperties(
          availableDevice, nullptr, &count, availableExtensions.data());
        
        bool deviceHasRequiredExtensions = true;
        
        for (auto &requiredExt : requiredDeviceExtensions) {
          bool foundExtension = false;
          
          for (auto &availableExt : availableExtensions) {
            if (strcmp(requiredExt, availableExt.extensionName) == 0) {
              foundExtension = true;
              break;
            }
          }
          
          if (!foundExtension) {
            deviceHasRequiredExtensions = false;
            break;
          }
        }
        
        if (!deviceHasRequiredExtensions) continue;
      }
      
      // Check for required format and colour space
      {
        uint32_t count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(availableDevice, surface, &count, nullptr);
        vector<VkSurfaceFormatKHR> formats(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(availableDevice, surface, &count, formats.data());

        bool found = false;
        for (auto &format : formats) {
          if (format.format == surfaceFormat
            && format.colorSpace == surfaceColorSpace) {
            found = true;
          }
        }
        
        if (!found) continue;
      }

      suitableDevices.push_back(availableDevice);
    }
    
    SDL_assert_release(suitableDevices.size() == 1);
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(suitableDevices[0], &properties);
    printf("\nChosen device: %s\n", properties.deviceName);
    return suitableDevices[0];
  }
  
  uint32_t getMemoryType(uint32_t memTypeBits, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
      if ((memTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) return i;
    }

    SDL_assert_release(false);
    return 0;
  }
  
  VkExtent2D getSurfaceExtent() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &capabilities);
    return capabilities.currentExtent;
  }
  
  vector<VkImage> getSwapchainImages() {
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    SDL_assert_release(imageCount == swapchainSize);
    vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());
    return images;
  }
  
  SwapchainFrame* getNextFrame(VkSemaphore imageAvailableSemaphore) {
    SDL_assert(imageAvailableSemaphore != VK_NULL_HANDLE);
    
    // Get next swapchain image
    // Get the next swapchain image and signal the semaphore.
    uint32_t swapchainIndex = INT32_MAX;
    auto result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX /* no timeout */, imageAvailableSemaphore, VK_NULL_HANDLE, &swapchainIndex);
    SDL_assert(result == VK_SUCCESS);
    
    return &swapchainFrames[swapchainIndex];
  }
}








