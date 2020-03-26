#include "graphics.h"
#include "settings.h"

namespace gfx {
  
  VkInstance               instance         = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debugMsgr        = VK_NULL_HANDLE;
  VkSurfaceKHR             surface          = VK_NULL_HANDLE;
  VkPhysicalDevice         physDevice       = VK_NULL_HANDLE;
  VkDevice                 device           = VK_NULL_HANDLE;
  VkDescriptorPool         descriptorPool   = VK_NULL_HANDLE;
  VkRenderPass             renderPass       = VK_NULL_HANDLE;
  VkQueue                  queue            = VK_NULL_HANDLE;
  int                      queueFamilyIndex = -1;
  VkCommandPool            commandPool      = VK_NULL_HANDLE;
  VkImageView              depthImageView   = VK_NULL_HANDLE;
  
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  SwapchainFrame swapchainFrames[swapchainSize];
  
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
        && families[familyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT
        && families[familyIndex].queueFlags & VK_QUEUE_TRANSFER_BIT) {
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
    enabledDeviceFeatures.samplerAnisotropy = VK_TRUE;
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
  
  void createBuffer(VkBufferUsageFlags usage, uint64_t dataSize, VkBuffer *bufferOut, VkDeviceMemory *memoryOut) {
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    auto result = vkCreateBuffer(device, &bufferInfo, nullptr, bufferOut);
    SDL_assert_release(result == VK_SUCCESS);
    
    *memoryOut = allocateAndBindMemory(*bufferOut);
  }
  
  void createVec3Buffer(const vector<vec3> &vec3s, VkBuffer *bufferOut, VkDeviceMemory *memoryOut) {
    
    uint64_t dataSize = sizeof(vec3s[0]) * vec3s.size();
    createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, dataSize, bufferOut, memoryOut);
    
    uint8_t *data = (uint8_t*)vec3s.data();
    setBufferMemory(*memoryOut, dataSize, data);
  }
  
  VkFramebuffer createFramebuffer(VkRenderPass renderPass, vector<VkImageView> attachments, uint32_t width, uint32_t height) {

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = (uint32_t)attachments.size();
    framebufferInfo.pAttachments = attachments.data();
    
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    
    framebufferInfo.layers = 1;
    
    VkFramebuffer framebuffer;
    auto result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer);
    SDL_assert_release(result == VK_SUCCESS);
    return framebuffer;
  }
  
  static void createSwapchainFrames() {
    auto images = getSwapchainImages();
    
    auto extent = getSurfaceExtent();
    
    for (int i = 0; i < images.size(); i++) {
      swapchainFrames[i].index = i;
      
      VkImage msaaImage;
      VkDeviceMemory msaaImageMemory;
      createImage(surfaceFormat, extent.width, extent.height, &msaaImage, &msaaImageMemory, MSAA_SETTING);
      
      swapchainFrames[i].msaaView = createImageView(msaaImage, surfaceFormat, VK_IMAGE_ASPECT_COLOR_BIT);
      
      swapchainFrames[i].resolvedView = createImageView(images[i], surfaceFormat, VK_IMAGE_ASPECT_COLOR_BIT);
      
      swapchainFrames[i].framebuffer = createFramebuffer(renderPass, {swapchainFrames[i].msaaView, swapchainFrames[i].resolvedView, depthImageView}, extent.width, extent.height);
      swapchainFrames[i].cmdBuffer = createCommandBuffer();
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
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Suitable for VkFramebuffer
    
    // If the graphics and surface queues are the same, no sharing is necessary.
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Opaque window
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // vsync
    // createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // vsync
    
    // Vulkan may not render all pixels if some are osbscured by other windows.
    createInfo.clipped = VK_TRUE;
    
    // I don't currently support swapchain recreation.
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    auto result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
    SDL_assert_release(result == VK_SUCCESS);
    
    createSwapchainFrames();
  }
  
  VkImageView createDepthImageAndView(uint32 width, uint32_t height, VkSampleCountFlagBits sampleCountFlag) {
    SDL_assert_release(commandPool != VK_NULL_HANDLE);
    
    VkImage image;
    VkDeviceMemory imageMemory;
    
    createImage(depthImageFormat, width, height, &image, &imageMemory, sampleCountFlag);

    return createImageView(image, depthImageFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
  }
  
  VkSubpassDependency createSubpassDependency() {
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    return dependency;
  }
  
  VkAttachmentDescription createAttachmentDescription(VkFormat format, bool clear, VkAttachmentStoreOp storeOp, VkImageLayout finalLayout, VkSampleCountFlagBits sampleCountFlag) {
    VkAttachmentDescription description = {};

    description.format = format;
    description.samples = sampleCountFlag;
    description.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    description.storeOp = storeOp;
    description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    description.finalLayout = finalLayout;
    
    return description;
  }
  
  static void createSubpass(VkSubpassDescription *descriptionOut, VkSubpassDependency *dependencyOut, vector<VkAttachmentDescription> *attachmentsOut, vector<VkAttachmentReference> *attachmentRefsOut) {
    
    // Hacky: attachmentRefsOut is passed out of this function on the stack to prevent its references in VkSubpassDescription from being deallocated before they're used. attachmentRefsOut doesn't need to be directly used by the caller of this function.
    
    attachmentsOut->resize(0);
    
    // MSAA color attachment
    attachmentsOut->push_back(createAttachmentDescription(surfaceFormat, true, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, MSAA_SETTING));
    
    // Resolved, single-sample-per-pixel color attachment
    attachmentsOut->push_back(createAttachmentDescription(surfaceFormat, false, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
    
    // Depth attachment
    attachmentsOut->push_back(createAttachmentDescription(depthImageFormat, true, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, MSAA_SETTING));
    
    // attachment references
    attachmentRefsOut->resize(0);
    
    VkAttachmentReference msaaColorAttachmentRef = {};
    msaaColorAttachmentRef.attachment = 0; // attachments[0]
    msaaColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentRefsOut->push_back(msaaColorAttachmentRef);
    
    VkAttachmentReference resolvedColorAttachmentRef = {};
    resolvedColorAttachmentRef.attachment = 1; // attachments[1]
    resolvedColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentRefsOut->push_back(resolvedColorAttachmentRef);
    
    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 2; // attachments[2]
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachmentRefsOut->push_back(depthAttachmentRef);

    memset(descriptionOut, 0, sizeof(VkSubpassDescription));
    descriptionOut->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    descriptionOut->colorAttachmentCount = 1;
    descriptionOut->pColorAttachments = &(*attachmentRefsOut)[0];
    descriptionOut->pResolveAttachments = &(*attachmentRefsOut)[1];
    descriptionOut->pDepthStencilAttachment = &(*attachmentRefsOut)[2];
    
    *dependencyOut = createSubpassDependency();
  }
  
  static VkRenderPass createRenderPass() {
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
  
  static VkDescriptorPool createDescriptorPool(uint32_t descriptorSetCount) {
    
    VkDescriptorPoolSize bufferPoolSize = {};
    bufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bufferPoolSize.descriptorCount = descriptorSetCount;
    
    VkDescriptorPoolSize samplerPoolSize = {};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = descriptorSetCount;
    
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = descriptorSetCount;
    poolInfo.flags = 0;
    
    VkDescriptorPoolSize sizes[] = { bufferPoolSize, samplerPoolSize };
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = sizes;
    
    VkDescriptorPool pool;
    auto result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
    SDL_assert_release(result == VK_SUCCESS);
    
    return pool;
  }
  
  static VkDescriptorSetLayout createDescriptorSetLayout(VkDescriptorType descriptorType) {
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.descriptorType = descriptorType;
    layoutBinding.binding = 0;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;
    
    VkDescriptorSetLayout layout;
    auto result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout);
    SDL_assert_release(result == VK_SUCCESS);
    
    return layout;
  }
  
  static void createDescriptorSet(VkDescriptorType descriptorType, const VkDescriptorBufferInfo *optionalBufferInfo, const VkDescriptorImageInfo *optionalImageInfo, VkDescriptorSet *descSetOut, VkDescriptorSetLayout *layoutOut) {
    
    *layoutOut = createDescriptorSetLayout(descriptorType);
    
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layoutOut;
    
    auto result = vkAllocateDescriptorSets(device, &allocInfo, descSetOut);
    SDL_assert_release(result == VK_SUCCESS);
    
    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = *descSetOut;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorType = descriptorType;
    writeDescriptorSet.descriptorCount = 1;
    
    if (descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
      writeDescriptorSet.pBufferInfo = optionalBufferInfo;
    } else if (descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
      writeDescriptorSet.pImageInfo = optionalImageInfo;
    } else SDL_assert_release(false); // Unsupported descriptor type.
    
    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
  }
  
  void createDescriptorSet(VkBuffer buffer, VkDescriptorSet *descSetOut, VkDescriptorSetLayout *layoutOut) {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;
    createDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo, nullptr, descSetOut, layoutOut);
  }
  
  void createDescriptorSet(VkImageView imageView, VkSampler sampler, VkDescriptorSet *descSetOut, VkDescriptorSetLayout *layoutOut) {
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;
    createDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &imageInfo, descSetOut, layoutOut);
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
    
    physDevice = getPhysicalDevice();
    SDL_assert_release(physDevice != VK_NULL_HANDLE);
    
    createDeviceAndQueue();
    SDL_assert_release(device != VK_NULL_HANDLE);
    SDL_assert_release(queue != VK_NULL_HANDLE);
    SDL_assert_release(queueFamilyIndex >= 0);
    
    createCommandPool();
    
    auto extent = getSurfaceExtent();
    depthImageView = createDepthImageAndView(extent.width, extent.height, MSAA_SETTING);
    
    renderPass = createRenderPass();
    
    createSwapchain();
    
    for (int i = 0; i < swapchainSize; i++) {
      SDL_assert_release(swapchainFrames[i].msaaView != VK_NULL_HANDLE);
      SDL_assert_release(swapchainFrames[i].resolvedView != VK_NULL_HANDLE);
    }
    
    descriptorPool = createDescriptorPool(1024);
  }
  
  void createImage(VkFormat format, uint32_t width, uint32_t height, VkImage *imageOut, VkDeviceMemory *memoryOut, VkSampleCountFlagBits sampleCountFlag) {
    
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    
    VkMemoryPropertyFlags memoryProperties;
    
    imageInfo.format = format;
    
    // Set format, usage, and memory properties.
    if (imageInfo.format == depthImageFormat) {
      imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      
      // This flag appears to be inessential (on macOS at least), but is recommended.
      // Might be good for color images too.
      memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    } else {
      imageInfo.usage = 0;
      imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Enable data transfers to this image
      imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT; // Enable shader usage via a sampler
      imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Enable render-to-image
      
      memoryProperties = 0;
    }
    
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1; // This creates a 2D image
    
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.samples = sampleCountFlag;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    auto result = vkCreateImage(device, &imageInfo, nullptr, imageOut);
    SDL_assert_release(result == VK_SUCCESS);
    
    *memoryOut = allocateAndBindMemory(*imageOut, memoryProperties);
  }
  
  void createColorImage(uint32_t width, uint32_t height, VkImage *imageOut, VkDeviceMemory *memoryOut) {
    createImage(surfaceFormat, width, height, imageOut, memoryOut);
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
  
  VkSampler createSampler() {
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    
    info.magFilter = VK_FILTER_LINEAR; // or NEAREST
    info.minFilter = VK_FILTER_LINEAR; // or NEAREST
    
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    
    info.anisotropyEnable = VK_TRUE;
    info.maxAnisotropy = 16;
    
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    
    info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    
    VkSampler sampler;
    auto result = vkCreateSampler(device, &info, nullptr, &sampler);
    SDL_assert_release(result == VK_SUCCESS);
    
    return sampler;
  }
  
  VkPipelineVertexInputStateCreateInfo allocVertexInputInfo(const vector<VkFormat> &attribFormats) {
    VkPipelineVertexInputStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    info.vertexBindingDescriptionCount = (uint32_t)attribFormats.size(); // I use only one binding per attribute.
    info.vertexAttributeDescriptionCount = (uint32_t)attribFormats.size();
    
    auto bindings = new VkVertexInputBindingDescription[attribFormats.size()];
    auto attribs = new VkVertexInputAttributeDescription[attribFormats.size()];
    
    for (int i = 0; i < attribFormats.size(); i++) {
      
      bindings[i].binding = i;
      bindings[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      
      // Set bindings[i].stride
      switch (attribFormats[i]) {
        case VK_FORMAT_R32G32B32_SFLOAT: bindings[i].stride = sizeof(vec3); break;
        case VK_FORMAT_R32G32_SFLOAT: bindings[i].stride = sizeof(vec2); break;
        default: SDL_assert_release(false); break; // Unsupported format!
      };
      
      attribs[i].binding = i; // bindings[i]
      attribs[i].location = i;
      attribs[i].format = attribFormats[i];
      attribs[i].offset = 0;
    }
    
    info.pVertexBindingDescriptions = bindings;
    info.pVertexAttributeDescriptions = attribs;
    
    return info;
  }
  
  void freeVertexInputInfo(VkPipelineVertexInputStateCreateInfo info) {
    delete [] info.pVertexBindingDescriptions;
    delete [] info.pVertexAttributeDescriptions;
  }
  
  static VkPipelineDepthStencilStateCreateInfo createDepthStencilInfo() {
    VkPipelineDepthStencilStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.depthTestEnable = VK_TRUE;
    info.depthWriteEnable = VK_TRUE;
    info.depthCompareOp = VK_COMPARE_OP_LESS; // Lower depth values mean closer to 'camera'
    info.depthBoundsTestEnable = VK_FALSE;
    info.stencilTestEnable = VK_FALSE;
    return info;
  }
  
  static VkPipelineRasterizationStateCreateInfo createRasterizationInfo(VkCullModeFlags cullMode) {
    VkPipelineRasterizationStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.depthClampEnable = VK_FALSE;
    info.rasterizerDiscardEnable = VK_FALSE;
    info.polygonMode = VK_POLYGON_MODE_FILL;
    info.lineWidth = 1;
    info.cullMode = cullMode;
    info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    info.depthBiasEnable = VK_FALSE;
    return info;
  }
  
  static VkPipelineColorBlendAttachmentState createColorBlendAttachment() {
    VkPipelineColorBlendAttachmentState attachment = {};
    
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    attachment.blendEnable = VK_FALSE;
    
    // If blendEnable were VK_TRUE, the below settings would perform standard alpha blending.
    
    // dstColor.rgb = (srcColor.rgb * srcColorBlendFactor) <colorBlendOp> (dstColor.rgb * dstColorBlendFactor);
    attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.colorBlendOp = VK_BLEND_OP_ADD;

    // dstColor.a = (srcColor.a * srcAlphaBlendFactor) <alphaBlendOp> (dstColor.a * dstAlphaBlendFactor);
    attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    return attachment;
  }
  
  static VkPipelineViewportStateCreateInfo allocViewportInfo(VkExtent2D extent) {
    
    VkPipelineViewportStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    
    VkViewport *viewport = new VkViewport;
    memset(viewport, 0, sizeof(VkViewport));
    viewport->x = 0;
    viewport->y = 0;
    viewport->width = (float)extent.width;
    viewport->height = (float)extent.height;
    viewport->minDepth = 0;
    viewport->maxDepth = 1;
    info.viewportCount = 1;
    info.pViewports = viewport;
    
    VkRect2D *scissor = new VkRect2D;
    memset(scissor, 0, sizeof(VkRect2D));
    scissor->offset.x = 0;
    scissor->offset.y = 0;
    
    scissor->extent = extent;
    info.scissorCount = 1;
    info.pScissors = scissor;
    
    return info;
  }
  
  static void freeViewportInfo(VkPipelineViewportStateCreateInfo info) {
    delete info.pViewports;
    delete info.pScissors;
  }
  
  VkPipelineLayout createPipelineLayout(VkDescriptorSetLayout descriptorSetLayouts[], uint32_t descriptorSetLayoutCount, uint32_t pushConstantSize) {
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    layoutInfo.setLayoutCount = descriptorSetLayoutCount;
    layoutInfo.pSetLayouts = descriptorSetLayouts;
    
    VkPushConstantRange pushConstRange = {};
    
    if (pushConstantSize > 0) {
      pushConstRange.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
      pushConstRange.offset = 0;
      pushConstRange.size = pushConstantSize;
      layoutInfo.pushConstantRangeCount = 1;
      layoutInfo.pPushConstantRanges = &pushConstRange;
    }
    
    VkPipelineLayout pipelineLayout;
    auto result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout);
    SDL_assert_release(result == VK_SUCCESS);
    
    return pipelineLayout;
  }
  
  static VkPipelineShaderStageCreateInfo createShaderStage(const char *spirVFilePath, VkShaderStageFlagBits stage) {
    auto spirV = loadBinaryFile(spirVFilePath);

    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = spirV.size();
    moduleInfo.pCode = (uint32_t*)spirV.data();

    VkShaderModule module = VK_NULL_HANDLE;
    SDL_assert_release(vkCreateShaderModule(device, &moduleInfo, nullptr, &module) == VK_SUCCESS);

    VkPipelineShaderStageCreateInfo stageInfo = {};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    stageInfo.stage = stage;

    stageInfo.module = module;
    stageInfo.pName = "main";

    return stageInfo;
  }
  
  VkPipeline createPipeline(VkPipelineLayout layout, const vector<VkFormat> &vertexAttribFormats, VkExtent2D extent, VkRenderPass renderPass, VkCullModeFlags cullMode, const char *vertexShaderPath, const char *fragmentShaderPath, VkSampleCountFlagBits sampleCountFlag) {
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    
    colorBlending.logicOpEnable = VK_FALSE;
    
    auto colorBlendAttachment = createColorBlendAttachment();
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.attachmentCount = 1;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    
    vector<VkPipelineShaderStageCreateInfo> shaderStages = {
      createShaderStage(vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT),
      createShaderStage(fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT)
    };
    pipelineInfo.stageCount = (int)shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    
    auto vertexInputInfo = allocVertexInputInfo(vertexAttribFormats);
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    
    auto viewportInfo = allocViewportInfo(extent);
    pipelineInfo.pViewportState = &viewportInfo;
    
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = createDepthStencilInfo();
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    
    VkPipelineRasterizationStateCreateInfo rasterInfo = createRasterizationInfo(cullMode);
    pipelineInfo.pRasterizationState = &rasterInfo;
    
    VkPipelineMultisampleStateCreateInfo multisamplingInfo = {};
    multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingInfo.sampleShadingEnable = VK_FALSE;
    multisamplingInfo.rasterizationSamples = sampleCountFlag;
    pipelineInfo.pMultisampleState = &multisamplingInfo;
    
    pipelineInfo.pColorBlendState = &colorBlending;
    
    pipelineInfo.layout = layout;
    
    pipelineInfo.renderPass = renderPass;
    
    pipelineInfo.subpass = 0;
    
    VkPipeline pipeline;
    SDL_assert_release(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) == VK_SUCCESS);
    
    // Clean up
    freeVertexInputInfo(vertexInputInfo);
    freeViewportInfo(viewportInfo);
    for (auto &stage : shaderStages) vkDestroyShaderModule(device, stage.module, nullptr);
    
    return pipeline;
  }
}








