#pragma once
#include "GraphicsFoundation.h"

class DrawCall;

class GraphicsPipeline {
public:
  const GraphicsFoundation *foundation;
  
  static const int swapchainSize = 2; // Double buffered
  
  // TODO: make these private
  VkFramebuffer framebuffers[swapchainSize];
  VkRenderPass renderPass;
  VkCommandPool commandPool;
  VkSemaphore imageAvailableSemaphore;
  VkSwapchainKHR swapchain;
  VkImage swapchainImages[swapchainSize];
  VkImageView swapchainViews[swapchainSize];
  VkSemaphore renderCompletedSemaphore;
  VkPipeline vkPipeline;
  VkPipelineLayout pipelineLayout;
  bool depthTestingEnabled;
  
  GraphicsPipeline(const GraphicsFoundation *foundation, bool depthTest);
  ~GraphicsPipeline();
  
  void submit(const DrawCall *drawCall);
  
  void present();
  
private:
  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;
  
  struct DrawCallData {
    VkCommandBuffer commandBuffers[swapchainSize];
  };
  
  vector<DrawCallData> drawCallDataToSubmit;
  
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




