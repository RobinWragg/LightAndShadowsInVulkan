#pragma once
#include "GraphicsFoundation.h"

class DrawCall {
public:
  VkBuffer vertexBuffer;
  uint32_t vertexCount;
  
  DrawCall(const GraphicsFoundation *foundation, const vector<vec3> &vertices);
  ~DrawCall();

private:
  const GraphicsFoundation *foundation = nullptr;
  VkDeviceMemory vertexBufferMemory;
  
  void createVertexBuffer(const vector<vec3> &vertices);
};