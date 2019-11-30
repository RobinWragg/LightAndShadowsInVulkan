#include "GraphicsFoundation.h"
#include <SDL2/SDL_vulkan.h>

enum class GraphicsFoundation::QueueType {
  GRAPHICS,
  SURFACE_SUPPORT
};

GraphicsFoundation::GraphicsFoundation(
  SDL_Window *window,
  PFN_vkDebugUtilsMessengerCallbackEXT debugCallback) {
  
  instance = createInstance(window);
  
  if (!layers.empty()) createDebugMessenger(debugCallback);
  
  auto result = SDL_Vulkan_CreateSurface(window, instance, &surface);
  SDL_assert_release(result == SDL_TRUE);
  
  physDevice = createPhysicalDevice(window);
  
  createDeviceAndQueues();
}

VkExtent2D GraphicsFoundation::getSurfaceExtent() const {
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &capabilities);
  return capabilities.currentExtent;
}

uint32_t GraphicsFoundation::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physDevice, &memoryProperties);

  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) return i;
  }

  SDL_assert_release(false);
  return 0;
}

void GraphicsFoundation::printAvailableInstanceLayers() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  printf("\nAvailable instance layers:\n");
  for (const auto& layer : availableLayers) printf("\t%s\n", layer.layerName);
}

VkInstance GraphicsFoundation::createInstance(SDL_Window *window) {
  if (!layers.empty()) printAvailableInstanceLayers();
  
  // Get required extensions from SDL
  unsigned int extensionCount;
  SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
  vector<const char*> extensions(extensionCount);
  SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());
  
  // Add debug utils if layers are being used
  if (!layers.empty()) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  
  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.enabledExtensionCount = (int)extensions.size();
  createInfo.ppEnabledExtensionNames = extensions.data();
  
  // Enable the layers
  createInfo.enabledLayerCount = (int)layers.size();
  createInfo.ppEnabledLayerNames = layers.data();
  
  printf("\nEnabled instance layers:\n");
  for (auto &layer : layers) printf("\t%s\n", layer);
  printf("\n");
  
  VkInstance instance;
  auto creationResult = vkCreateInstance(&createInfo, nullptr, &instance);
  SDL_assert_release(creationResult == VK_SUCCESS);
  
  return instance;
}

VkDebugUtilsMessengerEXT GraphicsFoundation::createDebugMessenger(
  PFN_vkDebugUtilsMessengerCallbackEXT callback) {
  
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

  createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
  // createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
  createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
  createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

  createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
  createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

  createInfo.pfnUserCallback = callback;

  auto createDebugUtilsMessenger
    = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  
  VkDebugUtilsMessengerEXT debugMsgr = VK_NULL_HANDLE;
  auto result = createDebugUtilsMessenger(instance, &createInfo, nullptr, &debugMsgr);
  SDL_assert_release(result == VK_SUCCESS);
  return debugMsgr;
}

VkPhysicalDevice GraphicsFoundation::createPhysicalDevice(SDL_Window *window) {
  
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
      
      for (auto &requiredExt : deviceExtensions) {
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
        if (format.format == this->surfaceFormat
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

VkDeviceQueueCreateInfo GraphicsFoundation::createQueueInfo(QueueType queueType) {
  SDL_assert_release(surface != VK_NULL_HANDLE);
  
  uint32_t familyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &familyCount, nullptr);
  std::vector<VkQueueFamilyProperties> families(familyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &familyCount, families.data());

  int selectedIndex = -1;

  // Choose the first queue family of the required type
  for (int familyIndex = 0; familyIndex < families.size(); familyIndex++) {
    switch (queueType) {
      case QueueType::GRAPHICS:
        if (families[familyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) selectedIndex = familyIndex;
      break;
      case QueueType::SURFACE_SUPPORT:
        VkBool32 hasSurfaceSupport;
        vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, familyIndex, surface, &hasSurfaceSupport);
        if (hasSurfaceSupport == VK_TRUE) selectedIndex = familyIndex;
      break;
      default: SDL_assert_release(false); break;
    }
    
    if (selectedIndex >= 0) break;
  }

  SDL_assert_release(selectedIndex >= 0);

  VkDeviceQueueCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  info.queueFamilyIndex = selectedIndex;
  info.queueCount = 1;
  
  static float defaultPriority = 1.0f;
  info.pQueuePriorities = &defaultPriority;
  
  return info;
}

void GraphicsFoundation::createDeviceAndQueues() {
  
  auto graphicsQueueInfo = createQueueInfo(QueueType::GRAPHICS);
  auto surfaceQueueInfo = createQueueInfo(QueueType::SURFACE_SUPPORT);
  vector<VkDeviceQueueCreateInfo> queueInfos = {graphicsQueueInfo};
  
  if (surfaceQueueInfo.queueFamilyIndex != queueInfos[0].queueFamilyIndex) {
    queueInfos.push_back(surfaceQueueInfo);
  }
  
  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  
  deviceCreateInfo.queueCreateInfoCount = (int)queueInfos.size();
  deviceCreateInfo.pQueueCreateInfos = queueInfos.data();
  
  VkPhysicalDeviceFeatures enabledDeviceFeatures = {};
  deviceCreateInfo.pEnabledFeatures = &enabledDeviceFeatures;
  
  // Enable extensions
  deviceCreateInfo.enabledExtensionCount = (int)deviceExtensions.size();
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
  
  // Enable validation layers for the deviceOut, same as the instance
  deviceCreateInfo.enabledLayerCount = (int)layers.size();
  deviceCreateInfo.ppEnabledLayerNames = layers.data();
  
  auto result = vkCreateDevice(physDevice, &deviceCreateInfo, nullptr, &device);
  SDL_assert_release(result == VK_SUCCESS);
  
  // Get handles to the new queue(s)
  int queueIndex = 0; // Only one queue per VkDeviceQueueCreateInfo was created, so this is 0.
  
  vkGetDeviceQueue(device, graphicsQueueInfo.queueFamilyIndex, queueIndex, &graphicsQueue);
  SDL_assert_release(graphicsQueue != VK_NULL_HANDLE);
  
  vkGetDeviceQueue(device, surfaceQueueInfo.queueFamilyIndex, queueIndex, &surfaceQueue);
  SDL_assert_release(surfaceQueue != VK_NULL_HANDLE);
  
  // TODO: Handle different queues (VK_SHARING_MODE_EXCLUSIVE etc)... graphics and surface queues may be different on other hardware
  SDL_assert_release(surfaceQueueInfo.queueFamilyIndex == 0);
  SDL_assert_release(graphicsQueue == surfaceQueue);
}







