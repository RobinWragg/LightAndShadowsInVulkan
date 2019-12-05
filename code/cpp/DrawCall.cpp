#include "DrawCall.h"

DrawCall::DrawCall(const GraphicsPipeline *pipeline, const vector<vec3> &vertices) {
  this->pipeline = pipeline;
  
  vertexCount = (uint32_t)vertices.size();
  createVertexBuffer(vertices);
}

DrawCall::~DrawCall() {
  vkDestroyBuffer(pipeline->foundation->device, vertexBuffer, nullptr);
  vkFreeMemory(pipeline->foundation->device, vertexBufferMemory, nullptr);
}

void DrawCall::createVertexBuffer(const vector<vec3> &vertices) {
  
  uint64_t dataSize = sizeof(vertices[0]) * vertices.size();
  pipeline->foundation->createVkBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, dataSize, &vertexBuffer, &vertexBufferMemory);
  
  uint8_t *data = (uint8_t*)vertices.data();
  pipeline->foundation->setMemory(vertexBufferMemory, dataSize, data);
}





