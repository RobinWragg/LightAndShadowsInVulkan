#include "DrawCall.h"

DrawCall::DrawCall(const GraphicsPipeline *pipeline, const vector<vec3> &vertices) {
  this->pipeline = pipeline;
  this->foundation = pipeline->foundation;
  
  vertexCount = (uint32_t)vertices.size();
  createVertexBuffer(vertices);
  
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

void DrawCall::createVertexBuffer(const vector<vec3> &vertices) {
  
  uint64_t dataSize = sizeof(vertices[0]) * vertices.size();
  foundation->createVkBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, dataSize, &vertexBuffer, &vertexBufferMemory);
  
  uint8_t *data = (uint8_t*)vertices.data();
  foundation->setMemory(vertexBufferMemory, dataSize, data);
}





