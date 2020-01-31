#include "graphics.h"

namespace gfx {
  
  VkInstance               instance         = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debugMsgr        = VK_NULL_HANDLE;
  VkSurfaceKHR             surface          = VK_NULL_HANDLE;
  VkPhysicalDevice         physDevice       = VK_NULL_HANDLE;
  VkDevice                 device           = VK_NULL_HANDLE;
  VkQueue                  queue            = VK_NULL_HANDLE;
  int                      queueFamilyIndex = -1;
  VkCommandPool            commandPool      = VK_NULL_HANDLE;
  VkImageView              depthImageView   = VK_NULL_HANDLE;
  
  VkSwapchainKHR swapchain                     = VK_NULL_HANDLE;
  VkImageView    swapchainViews[swapchainSize] = {VK_NULL_HANDLE};
  
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
  
  static void createDebugMessenger() {
    
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
    
    createDebugUtilsMessenger(instance, &createInfo, nullptr, &debugMsgr);
  }
  
  static void createCommandPool() {
    SDL_assert_release(queueFamilyIndex >= 0);
    
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.pNext = nullptr;
    
    auto result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    SDL_assert_release(result == VK_SUCCESS);
  }
  
  static VkInstance createInstance(SDL_Window *window) {
    
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
    
    vkCreateInstance(&createInfo, nullptr, &instance);
    
    return instance;
  }
  
  VkCommandBuffer createCommandBuffer() {
    VkCommandBufferAllocateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferInfo.commandPool = commandPool;
    bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferInfo.commandBufferCount = 1;
    
    VkCommandBuffer cmdBuffer;
    auto result = vkAllocateCommandBuffers(device, &bufferInfo, &cmdBuffer);
    SDL_assert(result == VK_SUCCESS);
    
    return cmdBuffer;
  }

  static VkDeviceQueueCreateInfo createQueueInfo() {
    
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

  static void createDeviceAndQueue() {
    
    VkDeviceQueueCreateInfo queueInfo = createQueueInfo();
    
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
    
    auto result = vkCreateDevice(physDevice, &deviceCreateInfo, nullptr, &device);
    SDL_assert_release(result == VK_SUCCESS);
    
    // Get a handle to the new queue
    vkGetDeviceQueue(device, queueInfo.queueFamilyIndex, 0, &queue);
    SDL_assert_release(queue != VK_NULL_HANDLE);
    
    queueFamilyIndex = queueInfo.queueFamilyIndex;
  }
  
  static VkDeviceMemory allocateMemory(VkMemoryRequirements reqs, VkMemoryPropertyFlags properties) {
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = reqs.size;
    allocInfo.memoryTypeIndex = getMemoryType(reqs.memoryTypeBits, properties);
    
    VkDeviceMemory memory = VK_NULL_HANDLE;
    auto result = vkAllocateMemory(device, &allocInfo, nullptr, &memory);
    SDL_assert_release(result == VK_SUCCESS);
    
    return memory;
  }
  
  static VkDeviceMemory allocateAndBindMemory(VkBuffer buffer) {
    VkMemoryRequirements reqs = {};
    vkGetBufferMemoryRequirements(device, buffer, &reqs);

    VkDeviceMemory memory = allocateMemory(reqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    auto result = vkBindBufferMemory(device, buffer, memory, 0);
    SDL_assert_release(result == VK_SUCCESS);
    
    return memory;
  }
  
  static VkDeviceMemory allocateAndBindMemory(VkImage image, VkMemoryPropertyFlags properties) {
    VkMemoryRequirements reqs = {};
    vkGetImageMemoryRequirements(device, image, &reqs);
    
    VkDeviceMemory memory = allocateMemory(reqs, properties);
    
    auto result = vkBindImageMemory(device, image, memory, 0);
    SDL_assert_release(result == VK_SUCCESS);
    
    return memory;
  }
  
  void createBuffer(VkBufferUsageFlagBits usage, uint64_t dataSize, VkBuffer *bufferOut, VkDeviceMemory *memoryOut) {
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    auto result = vkCreateBuffer(device, &bufferInfo, nullptr, bufferOut);
    SDL_assert_release(result == VK_SUCCESS);
    
    *memoryOut = allocateAndBindMemory(*bufferOut);
  }
  
  static void createSwapchainViews() {
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

    for (int i = 0; i < swapchainSize; i++) {
      swapchainViews[i] = createImageView(images[i], surfaceFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
  }
  
  static void createSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &capabilities);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, nullptr);
    vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, presentModes.data());

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = swapchainSize;
    createInfo.imageFormat = surfaceFormat;
    createInfo.imageColorSpace = surfaceColorSpace;
    createInfo.imageExtent = capabilities.currentExtent;
    createInfo.imageArrayLayers = 1; // 1 == not stereoscopic
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Suitable for VkFrameBuffer
    
    // If the graphics and surface queues are the same, no sharing is necessary.
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Opaque window
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // vsync
    
    // Vulkan may not renderall pixels if some are osbscured by other windows.
    createInfo.clipped = VK_TRUE;
    
    // I don't currently support swapchain recreation.
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    auto result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
    SDL_assert_release(result == VK_SUCCESS);
    
    createSwapchainViews();
  }
  
  static void createDepthImageAndView() {
    SDL_assert_release(commandPool != VK_NULL_HANDLE);
    
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    
    auto extent = getSurfaceExtent();
    createImage(true, extent.width, extent.height, &depthImage, &depthImageMemory);

    depthImageView = createImageView(depthImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
  }
  
  void createCoreHandles(SDL_Window *window) {
    instance = createInstance(window);
    SDL_assert_release(instance != VK_NULL_HANDLE);
    
    #ifdef DEBUG
      createDebugMessenger();
      SDL_assert_release(debugMsgr != VK_NULL_HANDLE);
    #endif
    
    auto result = SDL_Vulkan_CreateSurface(window, instance, &surface);
    SDL_assert_release(result == SDL_TRUE);
    SDL_assert_release(surface != VK_NULL_HANDLE);
    
    physDevice = getPhysicalDevice(window);
    SDL_assert_release(physDevice != VK_NULL_HANDLE);
    
    createDeviceAndQueue();
    SDL_assert_release(device != VK_NULL_HANDLE);
    SDL_assert_release(queue != VK_NULL_HANDLE);
    SDL_assert_release(queueFamilyIndex >= 0);
    
    createCommandPool();
    
    createSwapchain();
    SDL_assert_release(swapchain != VK_NULL_HANDLE);
    
    for (int i = 0; i < swapchainSize; i++) {
      SDL_assert_release(swapchainViews[i] != VK_NULL_HANDLE);
    }
    
    createDepthImageAndView();
  }
  
  void createImage(bool forDepthTesting, uint32_t width, uint32_t height, VkImage *imageOut, VkDeviceMemory *memoryOut) {
    
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    
    VkMemoryPropertyFlags memoryProperties;
    
    // Set format, usage, and memory properties.
    if (forDepthTesting) {
      imageInfo.format = VK_FORMAT_D32_SFLOAT;
      imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      
      // This flag appears to be inessential (on macOS at least), but is recommended.
      // Might be good for color images too.
      memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    } else {
      imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
      imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      
      memoryProperties = 0;
    }
    
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1; // This creates a 2D image
    
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    auto result = vkCreateImage(device, &imageInfo, nullptr, imageOut);
    SDL_assert_release(result == VK_SUCCESS);
    
    *memoryOut = allocateAndBindMemory(*imageOut, memoryProperties);
  }
  
  void createColorImage(uint32_t width, uint32_t height, VkImage *imageOut, VkDeviceMemory *memoryOut) {
    createImage(false, width, height, imageOut, memoryOut);
  }
  
  VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask) {
    
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    
    viewInfo.image = image;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    VkImageView view;
    auto result = vkCreateImageView(device, &viewInfo, nullptr, &view);
    SDL_assert_release(result == VK_SUCCESS);
    
    return view;
  }
  
  static VkSubpassDependency createSubpassDependency() {
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    return dependency;
  }
  
  static VkAttachmentDescription createAttachmentDescription(VkFormat format, VkAttachmentStoreOp storeOp, VkImageLayout finalLayout) {
    VkAttachmentDescription description = {};

    description.format = format;
    description.samples = VK_SAMPLE_COUNT_1_BIT;
    description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    description.storeOp = storeOp;
    description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    description.finalLayout = finalLayout;
    
    return description;
  }
  
  void createSubpass(VkSubpassDescription *descriptionOut, VkSubpassDependency *dependencyOut, vector<VkAttachmentDescription> *attachmentsOut, vector<VkAttachmentReference> *attachmentRefsOut) {
    
    // Hacky: attachmentRefsOut is passed out of this function on the stack to prevent its references in VkSubpassDescription from being deallocated before they're used. attachmentRefsOut doesn't need to be directly used by the caller of this function.
    
    attachmentsOut->resize(0);
    attachmentsOut->push_back(createAttachmentDescription(surfaceFormat, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
    attachmentsOut->push_back(createAttachmentDescription(VK_FORMAT_D32_SFLOAT, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
    
    // attachment references
    attachmentRefsOut->resize(0);
    
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0; // attachments[0]
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentRefsOut->push_back(colorAttachmentRef);
    
    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1; // attachments[1]
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachmentRefsOut->push_back(depthAttachmentRef);

    bzero(descriptionOut, sizeof(VkSubpassDescription));
    descriptionOut->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    descriptionOut->colorAttachmentCount = 1;
    descriptionOut->pColorAttachments = &(*attachmentRefsOut)[0];
    descriptionOut->pDepthStencilAttachment = &(*attachmentRefsOut)[1];
    
    *dependencyOut = createSubpassDependency();
  }
  
  VkRenderPass createRenderPass() {
    VkSubpassDescription subpassDesc = {};
    VkSubpassDependency subpassDep = {};
    vector<VkAttachmentDescription> attachments;
    vector<VkAttachmentReference> attachmentRefs;
    createSubpass(&subpassDesc, &subpassDep, &attachments, &attachmentRefs);
    
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDesc;
    
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDep;
    
    renderPassInfo.attachmentCount = (uint32_t)attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    
    VkRenderPass renderPass;
    SDL_assert_release(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) == VK_SUCCESS);
    
    return renderPass;
  }
}








