#include "DrawCall.h"

DrawCall::DrawCall(const GraphicsPipeline *pipeline, const vector<vec3> &vertices) {
  this->pipeline = pipeline;
  this->foundation = pipeline->foundation;
  
  vertexCount = (uint32_t)vertices.size();
  createVec3Buffer(vertices, &vertexBuffer, &vertexBufferMemory);
  
  DrawCallUniform identityUniform;
  identityUniform.matrix = identity<mat4>();
  
  for (int i = 0; i < GraphicsPipeline::swapchainSize; i++) {
    foundation->createVkBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(DrawCallUniform), &descriptorBuffers[i], &descriptorBuffersMemory[i]);
    pipeline->createDescriptorSet(pipeline->drawCallDescriptorLayout, descriptorBuffers[i], &descriptorSets[i]);
    
    // Set identity as default
    foundation->setMemory(descriptorBuffersMemory[i], sizeof(DrawCallUniform), &identityUniform);
  }
}

DrawCall::~DrawCall() {
  vkDestroyBuffer(foundation->device, vertexBuffer, nullptr);
  vkFreeMemory(foundation->device, vertexBufferMemory, nullptr);
  
  for (int i = 0; i < GraphicsPipeline::swapchainSize; i++) {
   vkDestroyBuffer(foundation->device, descriptorBuffers[i], nullptr);
   vkFreeMemory(foundation->device, descriptorBuffersMemory[i], nullptr); 
  }
}

void DrawCall::createVec3Buffer(const vector<vec3> &vec3s, VkBuffer *bufferOut, VkDeviceMemory *memoryOut) const {
  
  uint64_t dataSize = sizeof(vec3s[0]) * vec3s.size();
  foundation->createVkBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, dataSize, bufferOut, memoryOut);
  
  uint8_t *data = (uint8_t*)vec3s.data();
  foundation->setMemory(*memoryOut, dataSize, data);
}





