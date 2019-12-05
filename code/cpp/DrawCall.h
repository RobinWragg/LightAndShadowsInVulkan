#pragma once
#include "GraphicsPipeline.h"

class DrawCall {
public:
  VkBuffer vertexBuffer;
  uint32_t vertexCount;
  
  DrawCall(const GraphicsPipeline *pipeline, const vector<vec3> &vertices);
  ~DrawCall();

private:
  const GraphicsPipeline *pipeline = nullptr;
  VkDeviceMemory vertexBufferMemory;
  
  void createVertexBuffer(const vector<vec3> &vertices);
};