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
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderCompletedSemaphore;
  VkPipeline vkPipeline;
  VkPipelineLayout pipelineLayout;
  
  GraphicsPipeline();
  
  void submit(DrawCall *drawCall, const DrawCallUniform *uniform);
  
  void present(const PerFrameUniform *perFrameUniform);
  
private:
  
  struct Submission {
    DrawCall *drawCall;
    DrawCallUniform uniform;
  };
  vector<Submission> submissions;
  
  vector<uint8_t> loadBinaryFile(const char *filename);
  
  void createSemaphores();
  
  void setupDepthTesting();
  
  void fillCommandBuffer(SwapchainFrame *frame, const PerFrameUniform *perFrameUniform);
};




