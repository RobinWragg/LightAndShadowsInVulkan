#pragma once
#include "GraphicsFoundation.h"

struct PerFrameShaderData {
  mat4 matrix;
};

class DrawCall;

class GraphicsPipeline {
public:
  const GraphicsFoundation *foundation;
  
  static const int swapchainSize = 2; // Double buffered
  
  // TODO: make these private
  VkFramebuffer framebuffers[swapchainSize];
  VkRenderPass renderPass;
  VkCommandPool commandPool;
  VkCommandBuffer commandBuffers[swapchainSize];
  VkFence fences[swapchainSize];
  VkSemaphore imageAvailableSemaphore;
  VkSwapchainKHR swapchain;
  VkImage swapchainImages[swapchainSize];
  VkImageView swapchainViews[swapchainSize];
  VkSemaphore renderCompletedSemaphore;
  VkPipeline vkPipeline;
  VkPipelineLayout pipelineLayout;
  VkDescriptorSet descriptorSets[swapchainSize];
  bool depthTestingEnabled;
  
  GraphicsPipeline(const GraphicsFoundation *foundation, bool depthTest);
  ~GraphicsPipeline();
  
  void submit(DrawCall *drawCall);
  
  void present(PerFrameShaderData *perFrameData);
  
private:
  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;
  VkBuffer uniformBuffers[swapchainSize];
  VkDeviceMemory uniformBuffersMemory[swapchainSize];
  
  VkDescriptorSetLayout perFrameDescriptorSetLayout;
  VkDescriptorPool descriptorPool;
  
  vector<DrawCall*> submissions;
  
  VkPipelineShaderStageCreateInfo createShaderStage(const char *spirVFilePath, VkShaderStageFlagBits stage);
  
  vector<uint8_t> loadBinaryFile(const char *filename);
  
  void createSemaphores();
  
  void setupDepthTesting();
  
  void createCommandPool();
  
  void createSwapchain();
  
  void createSwapchainImagesAndViews();
  
  VkAttachmentDescription createAttachmentDescription(VkFormat format, VkAttachmentStoreOp storeOp, VkImageLayout finalLayout);
  
  void createRenderPass();
  
  void createFramebuffers();
  
  void createDescriptorSetLayout();
  
  void createVkPipeline();
  
  void createUniformBuffers();
  
  void createDescriptorPool();
  
  void createDescriptorSets();
  
  void createCommandBuffers();
  
  void fillCommandBuffer(uint32_t swapchainIndex);
  
  void createFences();
};




