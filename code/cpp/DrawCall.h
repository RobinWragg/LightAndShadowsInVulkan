#pragma once
#include "GraphicsPipeline.h"

class DrawCall {
public:
  VkBuffer vertexBuffer;
  uint32_t vertexCount;
  
  DrawCall(const GraphicsFoundation *foundation, const vector<vec3> &vertices);
  ~DrawCall();

private:
  const GraphicsFoundation *foundation = nullptr;
  VkDeviceMemory vertexBufferMemory;
  
  VkDescriptorSet descriptorSets[GraphicsPipeline::swapchainSize];
  VkBuffer        buffers[GraphicsPipeline::swapchainSize];
  VkDeviceMemory  buffersMemory[GraphicsPipeline::swapchainSize];
  
  void createVertexBuffer(const vector<vec3> &vertices);
};