#pragma once
#include "graphics.h"

using namespace gfx;

struct PerFrameUniform {
  mat4 matrix;
};

struct DrawCallUniform {
  mat4 matrix;
};

class DrawCall;

class GraphicsPipeline {
public:
  static const int maxDescriptors = 1024;
  
  VkFramebuffer framebuffers[swapchainSize];
  VkRenderPass renderPass;
  VkCommandBuffer commandBuffers[swapchainSize];
  VkFence fences[swapchainSize];
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderCompletedSemaphore;
  VkPipeline vkPipeline;
  VkPipelineLayout pipelineLayout;
  
  VkDescriptorSetLayout drawCallDescriptorLayout;
  
  GraphicsPipeline();
  ~GraphicsPipeline();
  
  void createDescriptorSet(VkDescriptorSetLayout layout, VkBuffer buffer, VkDescriptorSet *setOut) const;
  
  void submit(DrawCall *drawCall, const DrawCallUniform *uniform);
  
  void present(const PerFrameUniform *perFrameUniform);
  
private:
  VkDescriptorPool descriptorPool;
  
  VkDescriptorSetLayout perFrameDescriptorLayout;
  
  VkDescriptorSet       perFrameDescriptorSets[swapchainSize];
  VkBuffer              perFrameDescriptorBuffers[swapchainSize];
  VkDeviceMemory        perFrameDescriptorBuffersMemory[swapchainSize];
  
  struct Submission {
    DrawCall *drawCall;
    DrawCallUniform uniform;
  };
  vector<Submission> submissions;
  
  VkPipelineShaderStageCreateInfo createShaderStage(const char *spirVFilePath, VkShaderStageFlagBits stage);
  
  vector<uint8_t> loadBinaryFile(const char *filename);
  
  void createSemaphores();
  
  void setupDepthTesting();
  
  void createRenderPass();
  
  void createFramebuffers();
  
  void createDescriptorSetLayout(VkDescriptorSetLayout *layoutOut) const;
  
  void createVkPipeline();
  
  void createDescriptorPool();
  
  void createCommandBuffers();
  
  void fillCommandBuffer(uint32_t swapchainIndex);
  
  void createFences();
};




