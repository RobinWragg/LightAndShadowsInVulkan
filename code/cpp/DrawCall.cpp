#include "DrawCall.h"

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
  
  modelMatrix = glm::identity<mat4>();
  
  printf("Created draw call with %li vertices\n", positions.size());
}

void DrawCall::addToCmdBuffer(VkCommandBuffer cmdBuffer, VkPipelineLayout layout) {
  vkCmdPushConstants(cmdBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(modelMatrix), &modelMatrix);
  
  const int bufferCount = 2;
  VkBuffer buffers[bufferCount] = {
    positionBuffer,
    normalBuffer
  };
  VkDeviceSize offsets[bufferCount] = {0, 0};
  vkCmdBindVertexBuffers(cmdBuffer, 0, bufferCount, buffers, offsets);
  
  vkCmdDraw(cmdBuffer, vertexCount, 1, 0, 0);
}





