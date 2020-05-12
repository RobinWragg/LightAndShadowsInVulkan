#pragma once
#include "graphics.h"
#include "linear_algebra.h"

class DrawCall {
public:
  // Descriptor set information
  struct {
    mat4 worldMatrix = glm::identity<mat4>();
    
    // Phong material defaults
    float diffuseReflectionConst = 0.5;
    float specReflectionConst    = 0.5;
    int   specPowerConst         = 10;
  } descSetData;
  
  // This constructor builds normals based on positions
  // (see createNormalsFromPositions())
  DrawCall(const vector<vec3> &positions);
  
  // Specify normals explicitly
  DrawCall(
    const vector<vec3> &positions,
    const vector<vec3> &normals);
  
  // Specify normals and texture coordinates
  DrawCall(
    const vector<vec3> &positions,
    const vector<vec3> &normals,
    const vector<vec2> &texCoords);
  
  /// Submit rendering commands to a command buffer
  void addToCmdBuffer(
    VkCommandBuffer commandBuffer,
    VkPipelineLayout layout);

private:
  uint32_t vertexCount;
  
  // Per-vertex buffer handles
  VkBuffer positionBuffer = VK_NULL_HANDLE;
  VkDeviceMemory positionBufferMemory = VK_NULL_HANDLE;
  
  VkBuffer normalBuffer   = VK_NULL_HANDLE;
  VkDeviceMemory normalBufferMemory   = VK_NULL_HANDLE;
  
  VkBuffer texCoordBuffer = VK_NULL_HANDLE;
  VkDeviceMemory texCoordBufferMemory = VK_NULL_HANDLE;
  
  // Internal descriptor set handles
  VkBuffer        descSetBuffer       = VK_NULL_HANDLE;
  VkDeviceMemory  descSetBufferMemory = VK_NULL_HANDLE;
  VkDescriptorSet descSet             = VK_NULL_HANDLE;
  
  // Functionality shared by all constructors
  void initCommon(
    const vector<vec3> &positions,
    const vector<vec3> &normals);
  
  vector<vec3> createNormalsFromPositions(
    const vector<vec3> &positions);
};




