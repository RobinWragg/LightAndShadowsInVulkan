#include "graphics_creation.h"
#include <array>

vector<const char*> layers = {
  // "VK_LAYER_LUNARG_api_dump",
  "VK_LAYER_LUNARG_standard_validation",
  "VK_LAYER_KHRONOS_validation"
};

const auto requiredFormat = VK_FORMAT_B8G8R8A8_UNORM;
const auto requiredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

enum class QueueType {
  GRAPHICS,
  SURFACE_SUPPORT
};

void printAvailableInstanceLayers() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  printf("\nAvailable instance layers:\n");
  for (const auto& layer : availableLayers) printf("\t%s\n", layer.layerName);
}

VkInstance createInstance(SDL_Window *window, const vector<const char*> &layers) {
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
  
  VkInstance instance;
  auto creationResult = vkCreateInstance(&createInfo, nullptr, &instance);
  SDL_assert_release(creationResult == VK_SUCCESS);
  
  return instance;
}

VkDebugUtilsMessengerEXT  createDebugMessenger(VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callback) {
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

VkPhysicalDevice createPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, SDL_Window *window) {
  
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
      vkEnumerateDeviceExtensionProperties(availableDevice, nullptr, &count, availableExtensions.data());
      
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
        if (format.format == requiredFormat
          && format.colorSpace == requiredColorSpace) {
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

VkDeviceQueueCreateInfo createQueueInfo(QueueType queueType, VkSurfaceKHR surface, VkPhysicalDevice physDevice) {
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

void createDeviceAndQueues(
  VkPhysicalDevice physDevice,
  VkSurfaceKHR surface,
  VkDevice *deviceOut,
  VkQueue *graphicsQueueOut,
  VkQueue *surfaceQueueOut) {
  
  auto graphicsQueueInfo = createQueueInfo(QueueType::GRAPHICS, surface, physDevice);
  auto surfaceQueueInfo = createQueueInfo(QueueType::SURFACE_SUPPORT, surface, physDevice);
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
  
  auto result = vkCreateDevice(physDevice, &deviceCreateInfo, nullptr, deviceOut);
  SDL_assert_release(result == VK_SUCCESS);
  
  // Get handles to the new queue(s)
  int queueIndex = 0; // Only one queue per VkDeviceQueueCreateInfo was created, so this is 0.
  
  vkGetDeviceQueue(*deviceOut, graphicsQueueInfo.queueFamilyIndex, queueIndex, graphicsQueueOut);
  SDL_assert_release(*graphicsQueueOut != VK_NULL_HANDLE);
  
  vkGetDeviceQueue(*deviceOut, surfaceQueueInfo.queueFamilyIndex, queueIndex, surfaceQueueOut);
  SDL_assert_release(*surfaceQueueOut != VK_NULL_HANDLE);
  
  
  // TODO: Handle different queues (VK_SHARING_MODE_EXCLUSIVE etc)
  SDL_assert_release(surfaceQueueInfo.queueFamilyIndex == 0);
  SDL_assert_release(*graphicsQueueOut == *surfaceQueueOut);
}

GraphicsBaseHandles init2(
  SDL_Window *window,
  const vector<const char*> &layers,
  PFN_vkDebugUtilsMessengerCallbackEXT debugCallbackOut) {
  GraphicsBaseHandles handles;
  
  handles.instance = createInstance(window, layers);
  
  if (!layers.empty()) createDebugMessenger(handles.instance, debugCallbackOut);
  
  auto result = SDL_Vulkan_CreateSurface(window, handles.instance, &handles.surface);
  SDL_assert_release(result == SDL_TRUE);
  
  handles.physDevice = createPhysicalDevice(handles.instance, handles.surface, window);
  
  createDeviceAndQueues(
    handles.physDevice,
    handles.surface,
    &handles.device,
    &handles.graphicsQueue,
    &handles.surfaceQueue);
  
  return handles;
}

void destroy(GraphicsBaseHandles handles) {
    
  if (handles.debugMsgr != VK_NULL_HANDLE) {
    auto destroyDebugUtilsMessenger
      = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        handles.instance, "vkDestroyDebugUtilsMessengerEXT");
    destroyDebugUtilsMessenger(handles.instance, handles.debugMsgr, nullptr);
    handles.debugMsgr = VK_NULL_HANDLE;
  }
  
  // This destroys the queues too
  vkDestroyDevice(handles.device, nullptr);
  handles.device = VK_NULL_HANDLE;
  handles.graphicsQueue = VK_NULL_HANDLE;
  handles.surfaceQueue = VK_NULL_HANDLE;
  
  vkDestroySurfaceKHR(handles.instance, handles.surface, nullptr);
  handles.surface = VK_NULL_HANDLE;
  
  // This destroys the physical device too
  vkDestroyInstance(handles.instance, nullptr);
  handles.instance = VK_NULL_HANDLE;
  handles.physDevice = VK_NULL_HANDLE;
}

VkSwapchainKHR createSwapchain(const GraphicsBaseHandles &handles) {
  VkExtent2D extent = getExtent(handles);

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    handles.physDevice, handles.surface, &presentModeCount, nullptr);
  
  vector<VkPresentModeKHR> presentModes(presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    handles.physDevice, handles.surface, &presentModeCount, presentModes.data());

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = handles.surface;
  createInfo.minImageCount = 2; // Double buffered
  createInfo.imageFormat = requiredFormat;
  createInfo.imageColorSpace = requiredColorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1; // 1 == not stereoscopic
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Suitable for VkFrameBuffer
  
  // If the graphics and surface queues are the same, no sharing is necessary.
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    handles.physDevice, handles.surface, &capabilities);
  createInfo.preTransform = capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Opaque window
  createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // vsync
  
  // Vulkan may not renderall pixels if some are osbscured by other windows.
  createInfo.clipped = VK_TRUE;
  
  // I don't currently support swapchain recreation.
  createInfo.oldSwapchain = VK_NULL_HANDLE;
  
  VkSwapchainKHR swapchain;
  auto result = vkCreateSwapchainKHR(handles.device, &createInfo, nullptr, &swapchain);
  SDL_assert_release(result == VK_SUCCESS);
  
  return swapchain;
}

VkExtent2D getExtent(const GraphicsBaseHandles &handles) {
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(handles.physDevice, handles.surface, &capabilities);
  return capabilities.currentExtent;
}










