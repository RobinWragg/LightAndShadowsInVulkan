#pragma once
#include "GraphicsPipeline.h"

class DrawCall {
public:
  // TODO: make these private
  VkCommandBuffer commandBuffers[GraphicsPipeline::swapchainSize];
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  
  DrawCall(const GraphicsPipeline *pipeline, const vector<vec3> &vertices);

private:
  const GraphicsPipeline *pipeline = nullptr;
  VkDevice device = VK_NULL_HANDLE;
  
  void createVertexBuffer(const vector<vec3> &vertices);
  
  void createCommandBuffers(uint64_t vertexCount);
};