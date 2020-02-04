#pragma once
#include "main.h"
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vector>
#include "linear_algebra.h"
using namespace std;

namespace gfx {
  const VkFormat surfaceFormat            = VK_FORMAT_B8G8R8A8_UNORM;
  const VkColorSpaceKHR surfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  const vector<const char *> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
  const int swapchainSize = 2;
  
  struct SwapchainFrame {
    VkImageView view = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
    VkFence cmdBufferFence = VK_NULL_HANDLE;
    uint32_t index;
  };
  
  extern VkSwapchainKHR swapchain;
  extern SwapchainFrame swapchainFrames[swapchainSize];
  
  extern VkInstance               instance;
  extern VkDebugUtilsMessengerEXT debugMsgr;
  extern VkSurfaceKHR             surface;
  extern VkPhysicalDevice         physDevice;
  extern VkDevice                 device;
  extern VkRenderPass             renderPass;
  extern VkQueue                  queue;
  extern int                      queueFamilyIndex;
  extern VkCommandPool            commandPool;
  extern VkImageView              depthImageView;
  
  // creators (graphics_create.cpp)
  void createCoreHandles(SDL_Window *window);
  void createBuffer(VkBufferUsageFlagBits usage, uint64_t dataSize, VkBuffer *bufferOut, VkDeviceMemory *memoryOut);
  void createVec3Buffer(const vector<vec3> &vec3s, VkBuffer *bufferOut, VkDeviceMemory *memoryOut);
  void createColorImage(uint32_t width, uint32_t height, VkImage *imageOut, VkDeviceMemory *memoryOut);
  void createImage(bool forDepthTesting, uint32_t width, uint32_t height, VkImage *imageOut, VkDeviceMemory *memoryOut);
  VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask);
  VkSampler createSampler();
  VkCommandBuffer createCommandBuffer();
  VkPipelineLayout createPipelineLayout(VkDescriptorSetLayout descriptorSetLayouts[], uint32_t descriptorSetLayoutCount, uint32_t pushConstantSize);
  VkPipeline createPipeline(VkPipelineLayout layout, VkRenderPass renderPass, uint32_t vertexAttributeCount, const char *vertexShaderPath, const char *fragmentShaderPath);
    
  // getters (graphics_get.cpp)
  vector<const char*> getRequiredLayers();
  void                getAvailableInstanceLayers(vector<VkLayerProperties> *layerProperties);
  VkExtent2D          getSurfaceExtent();
  VkPhysicalDevice    getPhysicalDevice(SDL_Window *window);
  uint32_t            getMemoryType(uint32_t memTypeBits, VkMemoryPropertyFlags properties);
  vector<VkImage>     getSwapchainImages();
  SwapchainFrame*     getNextFrame(VkSemaphore imageAvailableSemaphore);
  
  // miscellaneous (graphics_misc.cpp)
  void setBufferMemory(VkDeviceMemory memory, uint64_t dataSize, const void *data);
  void setImageMemoryRGBA(VkImage, VkDeviceMemory memory, uint32_t width, uint32_t height, const float *data);
  void beginCommandBuffer(VkCommandBuffer cmdBuffer);
  void submitCommandBuffer(VkCommandBuffer cmdBuffer, VkSemaphore optionalWaitSemaphore = VK_NULL_HANDLE, VkPipelineStageFlags optionalWaitStage = 0, VkSemaphore optionalSignalSemaphore = VK_NULL_HANDLE, VkFence optionalFence = VK_NULL_HANDLE);
  void presentFrame(const SwapchainFrame *frame, VkSemaphore waitSemaphore);
}





