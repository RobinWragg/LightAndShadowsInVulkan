#include "GraphicsFoundation.h"

GraphicsFoundation::GraphicsFoundation(SDL_Window *window) {
  
  instance = gfx::createInstance(window, &debugMsgr);
  
  auto result = SDL_Vulkan_CreateSurface(window, instance, &surface);
  SDL_assert_release(result == SDL_TRUE);
  
  physDevice = gfx::getPhysicalDevice(window, instance, surface);
  
  gfx::createDeviceAndQueue(surface, physDevice, &device, &queue, &queueFamilyIndex);
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












