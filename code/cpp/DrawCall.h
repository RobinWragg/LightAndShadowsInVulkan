#pragma once
#include "GraphicsPipeline.h"

class DrawCall {
public:
  VkBuffer positionBuffer;
  VkBuffer normalBuffer;
  uint32_t vertexCount;
  
  VkDescriptorSet descriptorSets[swapchainSize];
  VkBuffer        descriptorBuffers[swapchainSize];
  VkDeviceMemory  descriptorBuffersMemory[swapchainSize];
  
  // Auto-make normals based on positions
  DrawCall(const GraphicsPipeline *pipeline, const vector<vec3> &vertices);
  
  // Pass normals in explicitly
  DrawCall(const GraphicsPipeline *pipeline, const vector<vec3> &positions, const vector<vec3> &normals);
  
  ~DrawCall();

private:
  const GraphicsPipeline *pipeline = nullptr;
  VkDeviceMemory positionBufferMemory;
  VkDeviceMemory normalBufferMemory;
  
  void initCommon(const GraphicsPipeline *pipeline, const vector<vec3> &positions, const vector<vec3> &normals);
  
  void createVec3Buffer(const vector<vec3> &vec3s, VkBuffer *bufferOut, VkDeviceMemory *memoryOut) const;
};




