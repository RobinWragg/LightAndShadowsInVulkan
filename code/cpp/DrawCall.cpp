#include "DrawCall.h"

DrawCall::DrawCall(const GraphicsPipeline *pipeline, const vector<vec3> &positions) {
  vector<vec3> normals;
  
  for (int32_t i = 0; i < positions.size(); i += 3) {
    vec3 a = positions[i] - positions[i+1];
    vec3 b = positions[i+2] - positions[i+1];
    
    vec3 crossProduct = cross(a, b);
    
    vec3 normal;
    if (length(crossProduct) == 0.0f) {
      normal = vec3(1, 0, 0);
    } else {
      normal = normalize(crossProduct);
    }
    
    normals.push_back(normal);
    normals.push_back(normal);
    normals.push_back(normal);
  }
  
  initCommon(pipeline, positions, normals);
}

DrawCall::DrawCall(const GraphicsPipeline *pipeline, const vector<vec3> &positions, const vector<vec3> &normals) {
  initCommon(pipeline, positions, normals);
}

DrawCall::~DrawCall() {
  vkDestroyBuffer(gfx::device, positionBuffer, nullptr);
  vkFreeMemory(gfx::device, positionBufferMemory, nullptr);
}

void DrawCall::initCommon(const GraphicsPipeline *pipeline, const vector<vec3> &positions, const vector<vec3> &normals) {
  this->pipeline = pipeline;
  
  vertexCount = (uint32_t)positions.size();
  createVec3Buffer(positions, &positionBuffer, &positionBufferMemory);
  createVec3Buffer(normals, &normalBuffer, &normalBufferMemory);
}

void DrawCall::createVec3Buffer(const vector<vec3> &vec3s, VkBuffer *bufferOut, VkDeviceMemory *memoryOut) const {
  
  uint64_t dataSize = sizeof(vec3s[0]) * vec3s.size();
  gfx::createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, dataSize, bufferOut, memoryOut);
  
  uint8_t *data = (uint8_t*)vec3s.data();
  gfx::setBufferMemory(*memoryOut, dataSize, data);
}





