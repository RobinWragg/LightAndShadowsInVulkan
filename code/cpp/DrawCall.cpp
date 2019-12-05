#include "DrawCall.h"

DrawCall::DrawCall(const GraphicsFoundation *foundation, const vector<vec3> &vertices) {
  this->foundation = foundation;
  
  vertexCount = (uint32_t)vertices.size();
  createVertexBuffer(vertices);
}

DrawCall::~DrawCall() {
  vkDestroyBuffer(foundation->device, vertexBuffer, nullptr);
  vkFreeMemory(foundation->device, vertexBufferMemory, nullptr);
}

void DrawCall::createVertexBuffer(const vector<vec3> &vertices) {
  
  uint64_t dataSize = sizeof(vertices[0]) * vertices.size();
  foundation->createVkBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, dataSize, &vertexBuffer, &vertexBufferMemory);
  
  uint8_t *data = (uint8_t*)vertices.data();
  foundation->setMemory(vertexBufferMemory, dataSize, data);
}





