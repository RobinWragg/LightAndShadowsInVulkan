#pragma once
#include "graphics.h"

using namespace gfx;

class DrawCall;

class GraphicsPipeline {
public:
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderCompletedSemaphore;
  VkPipeline vkPipeline;
  VkPipelineLayout pipelineLayout;
  
  GraphicsPipeline();
  
  void submit(const DrawCall *drawCall);
  
  void present(const mat4 &viewProjectionMatrix);
  
private:
  
  vector<DrawCall> submissions;
  
  vector<uint8_t> loadBinaryFile(const char *filename);
  
  void createSemaphores();
  
  void setupDepthTesting();
  
  void fillCommandBuffer(SwapchainFrame *frame, const mat4 &viewProjectionMatrix);
};




