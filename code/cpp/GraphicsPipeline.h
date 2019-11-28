#pragma once

#include "GraphicsFoundation.h"

class GraphicsPipeline {
public:
  // TODO: make these private
  vector<VkFramebuffer> framebuffers;
  VkRenderPass renderPass;
  VkCommandPool commandPool;
  bool enableDepthTesting;
  VkSemaphore imageAvailableSemaphore;
  VkSwapchainKHR swapchain;
  vector<VkImage> swapchainImages;
  vector<VkImageView> swapchainViews;
  VkSemaphore renderCompletedSemaphore;
  VkPipeline vkPipeline;
  VkPipelineLayout pipelineLayout;
  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
  
  GraphicsPipeline(const GraphicsFoundation *foundation, bool depthTest);
  
  VkExtent2D getSurfaceExtent();
  
private:
  const GraphicsFoundation *foundation;
  const int swapchainSize = 2; // Double buffered
  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;
  VkPipelineShaderStageCreateInfo createShaderStage(const char *spirVFilePath, VkShaderStageFlagBits stage);
  vector<uint8_t> loadBinaryFile(const char *filename);
  
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



