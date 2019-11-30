#pragma once
#include "GraphicsFoundation.h"

class GraphicsPipeline {
public:
  const GraphicsFoundation *foundation;
  
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
  
  GraphicsPipeline(const GraphicsFoundation *foundation, bool depthTest);
  
private:
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




