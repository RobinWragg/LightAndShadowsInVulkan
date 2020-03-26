#pragma once
#include "graphics.h"
#include "linear_algebra.h"

class DrawCall {
public:
  VkBuffer positionBuffer = VK_NULL_HANDLE;
  VkBuffer normalBuffer   = VK_NULL_HANDLE;
  VkBuffer texCoordBuffer = VK_NULL_HANDLE;
  uint32_t vertexCount;
  
  mat4 worldMatrix;
  VkBuffer                     worldMatrixBuffer        = VK_NULL_HANDLE;
  VkDeviceMemory               worldMatrixBufferMemory  = VK_NULL_HANDLE;
  VkDescriptorSet              worldMatrixDescSet       = VK_NULL_HANDLE;
  static VkDescriptorSetLayout worldMatrixDescSetLayout;
  
  DrawCall() {}
  
  // Auto-make normals based on positions
  DrawCall(const vector<vec3> &positions);
  
  // Pass normals in explicitly
  DrawCall(const vector<vec3> &positions, const vector<vec3> &normals);
  
  DrawCall(const vector<vec3> &positions, const vector<vec3> &normals, const vector<vec2> &texCoords);
  
  void addToCmdBuffer(VkCommandBuffer cmdBuffer, VkPipelineLayout layout);

private:
  VkDeviceMemory positionBufferMemory = VK_NULL_HANDLE;
  VkDeviceMemory normalBufferMemory   = VK_NULL_HANDLE;
  VkDeviceMemory texCoordBufferMemory = VK_NULL_HANDLE;
  
  void initCommon(const vector<vec3> &positions, const vector<vec3> &normals);
  vector<vec3> createNormalsFromPositions(const vector<vec3> &positions);
};




