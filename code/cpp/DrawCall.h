#pragma once
#include "GraphicsPipeline.h"

class DrawCall {
public:
  VkBuffer vertexBuffer;
  uint32_t vertexCount;
  
  VkDescriptorSet descriptorSets[GraphicsPipeline::swapchainSize];
  VkBuffer        descriptorBuffers[GraphicsPipeline::swapchainSize];
  VkDeviceMemory  descriptorBuffersMemory[GraphicsPipeline::swapchainSize];
  
  DrawCall(const GraphicsPipeline *pipeline, const vector<vec3> &vertices);
  ~DrawCall();

private:
  const GraphicsFoundation *foundation = nullptr;
  const GraphicsPipeline *pipeline = nullptr;
  VkDeviceMemory vertexBufferMemory;
  
  void createVertexBuffer(const vector<vec3> &vertices);
};