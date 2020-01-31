#pragma once

#include "main.h"
#include <vulkan/vulkan.h>

namespace gfx {
  
  const VkFormat surfaceFormat            = VK_FORMAT_B8G8R8A8_UNORM;
  const VkColorSpaceKHR surfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  const vector<const char *> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
  const int swapchainSize = 2;
  
  extern VkInstance               instance;
  extern VkDebugUtilsMessengerEXT debugMsgr;
  extern VkSurfaceKHR             surface;
  extern VkPhysicalDevice         physDevice;
  extern VkDevice                 device;
  extern VkQueue                  queue;
  extern int                      queueFamilyIndex;
  extern VkCommandPool            commandPool;
  extern VkImageView              depthImageView;
  
  extern VkSwapchainKHR swapchain;
  extern VkImageView    swapchainViews[swapchainSize];
  
  // creators
  void            createCoreHandles(SDL_Window *window);
  void            createBuffer(VkBufferUsageFlagBits usage, uint64_t dataSize, VkBuffer *bufferOut, VkDeviceMemory *memoryOut);
  void            createColorImage(uint32_t width, uint32_t height, VkImage *imageOut, VkDeviceMemory *memoryOut);
  void            createImage(bool forDepthTesting, uint32_t width, uint32_t height, VkImage *imageOut, VkDeviceMemory *memoryOut);
  VkImageView     createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask);
  void            createSubpass(VkSubpassDescription *descriptionOut, VkSubpassDependency *dependencyOut, vector<VkAttachmentDescription> *attachmentsOut, vector<VkAttachmentReference> *attachmentRefsOut);
  VkCommandBuffer createCommandBuffer();
  VkRenderPass    createRenderPass();
  
  // getters
  vector<const char*> getRequiredLayers();
  void                getAvailableInstanceLayers(vector<VkLayerProperties> *layerProperties);
  VkExtent2D          getSurfaceExtent();
  VkPhysicalDevice    getPhysicalDevice(SDL_Window *window);
  uint32_t            getMemoryType(uint32_t memTypeBits, VkMemoryPropertyFlags properties);
  
  // miscellaneous
  void setBufferMemory(VkDeviceMemory memory, uint64_t dataSize, const void *data);
  void setImageMemoryRGBA(VkImage, VkDeviceMemory memory, uint32_t width, uint32_t height, const float *data);
  void beginCommandBuffer(VkCommandBuffer cmdBuffer);
  void submitCommandBuffer(VkCommandBuffer cmdBuffer, VkSemaphore optionalWaitSemaphore = VK_NULL_HANDLE, VkPipelineStageFlags optionalWaitStage = 0, VkSemaphore optionalSignalSemaphore = VK_NULL_HANDLE, VkFence optionalFence = VK_NULL_HANDLE);
}





