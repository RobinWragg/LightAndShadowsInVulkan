#include "graphics.h"

namespace gfx {
  
  VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT msgType, const VkDebugUtilsMessengerCallbackDataEXT *data, void *pUserData) {

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
  
  static VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) {
    
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
    
    VkDebugUtilsMessengerEXT debugMsgr;
    auto result = createDebugUtilsMessenger(instance, &createInfo, nullptr, &debugMsgr);
    SDL_assert_release(result == VK_SUCCESS);
    return debugMsgr;
  }
  
  VkInstance createInstance(SDL_Window *window, VkDebugUtilsMessengerEXT *optionalDebugMsgrOut) {
    
    // Print available layers
    printf("\nAvailable instance layers:\n");
    vector<VkLayerProperties> availableLayers;
    getAvailableInstanceLayers(&availableLayers);
    for (const auto& layer : availableLayers) printf("\t%s\n", layer.layerName);
    
    // Get required extensions from SDL
    unsigned int extensionCount;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
    vector<const char*> extensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());
    
    #ifdef DEBUG
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.enabledExtensionCount = (int)extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    // Enable the layers
    auto layers = getRequiredLayers();
    createInfo.enabledLayerCount = (int)layers.size();
    createInfo.ppEnabledLayerNames = layers.data();
    
    printf("\nEnabled instance layers:\n");
    for (auto &layer : layers) printf("\t%s\n", layer);
    printf("\n");
    
    VkInstance instance;
    auto creationResult = vkCreateInstance(&createInfo, nullptr, &instance);
    SDL_assert_release(creationResult == VK_SUCCESS);
    
    #ifdef DEBUG
      *optionalDebugMsgrOut = createDebugMessenger(instance);
    #endif
    
    return instance;
  }

  static void createQueueInfo(VkSurfaceKHR surface, VkPhysicalDevice physDevice, VkDeviceQueueCreateInfo *infoOut) {
    SDL_assert_release(surface != VK_NULL_HANDLE);
    
    bzero(infoOut, sizeof(*infoOut));
    
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
    
    infoOut->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    infoOut->queueFamilyIndex = familyIndex;
    infoOut->queueCount = 1;
    
    static float defaultPriority = 1.0f;
    infoOut->pQueuePriorities = &defaultPriority;
  }

  void createDeviceAndQueue(VkSurfaceKHR surface, VkPhysicalDevice physDevice, VkDevice *deviceOut, VkQueue *queueOut, int *queueFamilyIndexOut) {
    
    VkDeviceQueueCreateInfo queueInfo;
    createQueueInfo(surface, physDevice, &queueInfo);
    
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueInfo;
    
    VkPhysicalDeviceFeatures enabledDeviceFeatures = {};
    deviceCreateInfo.pEnabledFeatures = &enabledDeviceFeatures;
    
    // Enable extensions
    deviceCreateInfo.enabledExtensionCount = (int)requiredDeviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
    
    // Enable validation layers for the device, same as the instance
    auto requiredLayers = getRequiredLayers();
    deviceCreateInfo.enabledLayerCount = (int)requiredLayers.size();
    deviceCreateInfo.ppEnabledLayerNames = requiredLayers.data();
    
    auto result = vkCreateDevice(physDevice, &deviceCreateInfo, nullptr, deviceOut);
    SDL_assert_release(result == VK_SUCCESS);
    
    // Get a handle to the new queue
    vkGetDeviceQueue(*deviceOut, queueInfo.queueFamilyIndex, 0, queueOut);
    SDL_assert_release(queueOut != VK_NULL_HANDLE);
    
    *queueFamilyIndexOut = queueInfo.queueFamilyIndex;
  }
}








