#pragma once
#include "GraphicsFoundation.h"

struct PerFrameUniform {
  mat4 matrix;
};

struct DrawCallUniform {
  mat4 matrix;
};

class DrawCall;

class GraphicsPipeline {
public:
  const GraphicsFoundation *foundation;
  
  static const int swapchainSize = 2; // Double buffered
  static const int maxDescriptors = 1024;
  
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
  bool depthTestingEnabled;
  
  GraphicsPipeline(const GraphicsFoundation *foundation, bool depthTest);
  ~GraphicsPipeline();
  
  void submit(DrawCall *drawCall);
  
  void present(const PerFrameUniform *perFrameUniform);
  
private:
  VkImage        depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView    depthImageView;
  
  VkDescriptorPool descriptorPool;
  
  uint32_t              drawCallDescriptorBinding;
  VkDescriptorSetLayout drawCallDescriptorLayout;
  VkDeviceSize          drawCallDescriptorBufferSize;
  
  uint32_t              perFrameDescriptorBinding;
  VkDescriptorSetLayout perFrameDescriptorLayout;
  VkDeviceSize          perFrameDescriptorBufferSize;
  
  VkDescriptorSet       perFrameDescriptorSets[swapchainSize];
  VkBuffer              perFrameDescriptorBuffers[swapchainSize];
  VkDeviceMemory        perFrameDescriptorBuffersMemory[swapchainSize];
  
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
  
  void createDescriptorSetLayout(int bindingIndex, VkDescriptorSetLayout *layoutOut) const;
  
  void createVkPipeline();
  
  void createDescriptorPool();
  
  void createDescriptorSet(VkDescriptorSetLayout layout, int bindingIndex, VkBuffer buffer, VkDescriptorSet *setOut) const;
  
  void createCommandBuffers();
  
  void fillCommandBuffer(uint32_t swapchainIndex);
  
  void createFences();
};




