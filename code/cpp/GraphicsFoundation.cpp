#include "GraphicsFoundation.h"

GraphicsFoundation::GraphicsFoundation(SDL_Window *window) {
  
  instance = createInstance(window);
  
  if (!layers.empty()) createDebugMessenger();
  
  auto result = SDL_Vulkan_CreateSurface(window, instance, &surface);
  SDL_assert_release(result == SDL_TRUE);
  
  physDevice = createPhysicalDevice(window);
  
  createDeviceAndQueue();
}

GraphicsFoundation::~GraphicsFoundation() {
  vkDestroyDevice(device, nullptr); // Also destroys the queues
  
  vkDestroySurfaceKHR(instance, surface, nullptr);
  
  auto destroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  destroyDebugUtilsMessenger(instance, debugMsgr, nullptr);
  
  vkDestroyInstance(instance, nullptr); // Also destroys all physical devices
}

void GraphicsFoundation::createVkBuffer(VkBufferUsageFlagBits usage, uint64_t dataSize, VkBuffer *bufferOut, VkDeviceMemory *memoryOut) const {
  
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = dataSize;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  
  auto result = vkCreateBuffer(device, &bufferInfo, nullptr, bufferOut);
  SDL_assert(result == VK_SUCCESS);
  
  VkMemoryRequirements memoryReqs;
  vkGetBufferMemoryRequirements(device, *bufferOut, &memoryReqs);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memoryReqs.size;
  allocInfo.memoryTypeIndex = findMemoryType(memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  result = vkAllocateMemory(device, &allocInfo, nullptr, memoryOut);
  SDL_assert(result == VK_SUCCESS);
  result = vkBindBufferMemory(device, *bufferOut, *memoryOut, 0);
  SDL_assert(result == VK_SUCCESS);
}

void GraphicsFoundation::setMemory(VkDeviceMemory memory, uint64_t dataSize, const void *data) const {
  
  uint8_t *mappedMemory;
  auto result = vkMapMemory(device, memory, 0, dataSize, 0, (void**)&mappedMemory);
  SDL_assert(result == VK_SUCCESS);
      
  memcpy(mappedMemory, data, dataSize);
  vkUnmapMemory(device, memory);
}

VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsFoundation::debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT severity,
  VkDebugUtilsMessageTypeFlagsEXT msgType,
  const VkDebugUtilsMessengerCallbackDataEXT *data,
  void *pUserData) {

  printf("\n");

  switch (severity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: printf("verbose, "); break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: printf("info, "); break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: printf("WARNING, "); break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: printf("ERROR, "); break;
  default: printf("unknown, "); break;
  };

  switch (msgType) {
  case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: printf("general: "); break;
  case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: printf("validation: "); break;
  case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: printf("performance: "); break;
  default: printf("unknown: "); break;
  };

  printf("%s (%i objects reported)\n", data->pMessage, data->objectCount);
  fflush(stdout);

  switch (severity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    SDL_assert_release(false);
    break;
  default: break;
  };

  return VK_FALSE;
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

VkDebugUtilsMessengerEXT GraphicsFoundation::createDebugMessenger() {
  
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

  createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
  // createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
  createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
  createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

  createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
  createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

  createInfo.pfnUserCallback = debugCallback;

  auto createDebugUtilsMessenger
    = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  
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

VkDeviceQueueCreateInfo GraphicsFoundation::createQueueInfo() {
  SDL_assert_release(surface != VK_NULL_HANDLE);
  
  uint32_t familyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &familyCount, nullptr);
  std::vector<VkQueueFamilyProperties> families(familyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &familyCount, families.data());


  // Choose the first queue family of the required type
  int familyIndex;
  for (familyIndex = 0; familyIndex < families.size(); familyIndex++) {
    VkBool32 hasSurfaceSupport;
    vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, familyIndex, surface, &hasSurfaceSupport);
    
    if (hasSurfaceSupport == VK_TRUE
      && families[familyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      break;
    }
  }

  SDL_assert_release(familyIndex < families.size());

  VkDeviceQueueCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  info.queueFamilyIndex = familyIndex;
  info.queueCount = 1;
  
  static float defaultPriority = 1.0f;
  info.pQueuePriorities = &defaultPriority;
  
  return info;
}

void GraphicsFoundation::createDeviceAndQueue() {
  
  auto queueInfo = createQueueInfo();
  queueFamilyIndex = queueInfo.queueFamilyIndex;
  
  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pQueueCreateInfos = &queueInfo;
  
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
  
  // Get a handle to the new queue
  vkGetDeviceQueue(device, queueInfo.queueFamilyIndex, 0, &queue);
  SDL_assert_release(queue != VK_NULL_HANDLE);
}







