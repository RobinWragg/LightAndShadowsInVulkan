#include "DrawCall.h"

VkDescriptorSetLayout DrawCall::worldMatrixDescSetLayout = VK_NULL_HANDLE;

DrawCall::DrawCall(const vector<vec3> &positions) {
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
  
  initCommon(positions, normals);
}

DrawCall::DrawCall(const vector<vec3> &positions, const vector<vec3> &normals) {
  initCommon(positions, normals);
}

void DrawCall::initCommon(const vector<vec3> &positions, const vector<vec3> &normals) {
  vertexCount = (uint32_t)positions.size();
  gfx::createVec3Buffer(positions, &positionBuffer, &positionBufferMemory);
  gfx::createVec3Buffer(normals, &normalBuffer, &normalBufferMemory);
  
  worldMatrix = glm::identity<mat4>();
  gfx::createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(worldMatrix), &worldMatrixBuffer, &worldMatrixBufferMemory);
  gfx::createDescriptorSet(worldMatrixBuffer, &worldMatrixDescSet, &worldMatrixDescSetLayout);
  
  printf("Created draw call with %i vertices\n", (int)positions.size());
}

void DrawCall::addToCmdBuffer(VkCommandBuffer cmdBuffer, VkPipelineLayout layout) {
  
  gfx::setBufferMemory(worldMatrixBufferMemory, sizeof(worldMatrix), &worldMatrix);
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &worldMatrixDescSet, 0, nullptr);
  
  const int bufferCount = 2;
  VkBuffer buffers[bufferCount] = {
    positionBuffer,
    normalBuffer
  };
  VkDeviceSize offsets[bufferCount] = {0, 0};
  vkCmdBindVertexBuffers(cmdBuffer, 0, bufferCount, buffers, offsets);
  
  vkCmdDraw(cmdBuffer, vertexCount, 1, 0, 0);
}





