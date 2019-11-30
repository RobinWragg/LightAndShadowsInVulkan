#pragma once
#include "GraphicsFoundation.h"

class GraphicsPipeline {
public:
  const GraphicsFoundation *foundation;
  
  static const int swapchainSize = 2; // Double buffered
  
  // TODO: make these private
  VkFramebuffer framebuffers[swapchainSize];
  VkRenderPass renderPass;
  VkCommandPool commandPool;
  bool enableDepthTesting;
  VkSemaphore imageAvailableSemaphore;
  VkSwapchainKHR swapchain;
  VkImage swapchainImages[swapchainSize];
  VkImageView swapchainViews[swapchainSize];
  VkSemaphore renderCompletedSemaphore;
  VkPipeline vkPipeline;
  VkPipelineLayout pipelineLayout;
  
  GraphicsPipeline(const GraphicsFoundation *foundation, bool depthTest);
  
private:
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




