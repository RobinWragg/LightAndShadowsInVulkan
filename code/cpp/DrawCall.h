#pragma once
#include "GraphicsPipeline.h"

class DrawCall {
public:
  VkBuffer positionBuffer;
  VkBuffer normalBuffer;
  uint32_t vertexCount;
  
  // Auto-make normals based on positions
  DrawCall(const vector<vec3> &positions);
  
  // Pass normals in explicitly
  DrawCall(const vector<vec3> &positions, const vector<vec3> &normals);
  
  ~DrawCall();

private:
  VkDeviceMemory positionBufferMemory;
  VkDeviceMemory normalBufferMemory;
  
  void initCommon(const vector<vec3> &positions, const vector<vec3> &normals);
  
  void createVec3Buffer(const vector<vec3> &vec3s, VkBuffer *bufferOut, VkDeviceMemory *memoryOut) const;
};




