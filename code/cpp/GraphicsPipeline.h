#pragma once

#include "GraphicsFoundation.h"

class GraphicsPipeline {
public:
  // TODO: make these private
  VkSwapchainKHR swapchain;
  vector<VkFramebuffer> framebuffers;
  const GraphicsFoundation *foundation;
  const int swapchainSize = 2; // Double buffered
  vector<VkImage> swapchainImages;
  vector<VkImageView> swapchainViews;
  VkRenderPass renderPass;
  VkCommandPool commandPool;
  bool enableDepthTesting;
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderCompletedSemaphore;
  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;
  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
  VkPipelineShaderStageCreateInfo createShaderStage(const char *spirVFilePath, VkShaderStageFlagBits stage);
  vector<uint8_t> loadBinaryFile(const char *filename);
  VkPipeline vkPipeline;
  VkPipelineLayout pipelineLayout;
  // VkRenderPass renderPass;
  // VkSwapchainKHR swapchain;
  
  GraphicsPipeline(const GraphicsFoundation *foundation, bool depthTest);
  
  VkExtent2D getSurfaceExtent();
  
private:
  void createSemaphores();
  
  void setupDepthTesting();
  
  void createCommandPool();
  
  void  createSwapchain();
  
  void createSwapchainImagesAndViews();
  
  VkAttachmentDescription createAttachmentDescription(VkFormat format, VkAttachmentStoreOp storeOp, VkImageLayout finalLayout);
  
  void createRenderPass();
  
  void createFramebuffers();
  
  void createVkPipeline();
};




