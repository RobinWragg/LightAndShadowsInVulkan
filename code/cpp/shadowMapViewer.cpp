#include "shadowMapViewer.h"
#include "main.h"
#include "graphics.h"
#include "scene.h"

namespace shadowMapViewer {
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline       pipeline       = VK_NULL_HANDLE;
  
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  
  VkBuffer matrixBuffer;
  VkDeviceMemory matrixBufferMemory;
  VkDescriptorSetLayout matrixDescriptorSetLayout;
  VkDescriptorSet matrixDescriptorSet;
  
  vector<vec3> vertices = {
    vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 1, 0),
    vec3(0, 1, 0), vec3(1, 0, 0), vec3(1, 1, 0)
  };
  
  ShadowMap *shadowMap;
  
  void init(ShadowMap *shadowMap_) {
    shadowMap = shadowMap_;
    
    // Create matrix handles
    gfx::createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(mat4), &matrixBuffer, &matrixBufferMemory);
    gfx::createDescriptorSet(matrixBuffer, &matrixDescriptorSet, &matrixDescriptorSetLayout);
    
    // Create pipeline
    VkDescriptorSetLayout descSetLayouts[] = {matrixDescriptorSetLayout, shadowMap->samplerDescriptorSetLayout};
    pipelineLayout = gfx::createPipelineLayout(descSetLayouts, 2, sizeof(mat4));
    pipeline = gfx::createPipeline(pipelineLayout, gfx::getSurfaceExtent(), gfx::renderPass, VK_CULL_MODE_BACK_BIT, 1, "shadowMapViewer.vert.spv", "shadowMapViewer.frag.spv");
    
    gfx::createVec3Buffer(vertices, &vertexBuffer, &vertexBufferMemory);
  }
  
  void renderQuad(VkCommandBuffer cmdBuffer) {
    float size = 0.7;
    
    mat4 matrix = glm::identity<mat4>();
    VkExtent2D extent = gfx::getSurfaceExtent();
    float aspectRatio = extent.width / (float)extent.height;
    matrix = scale(matrix, vec3(1 / aspectRatio, 1, 1));
    matrix = translate(matrix, vec3(-aspectRatio, 1 - size, 0));
    matrix = scale(matrix, vec3(size, size, 1));
    
    gfx::setBufferMemory(matrixBufferMemory, sizeof(matrix), &matrix);
    
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &matrixDescriptorSet, 0, nullptr);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &shadowMap->samplerDescriptorSet, 0, nullptr);
    
    VkDeviceSize vertexBufferOffset = 0;
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
    
    vkCmdDraw(cmdBuffer, (int)vertices.size(), 1, 0, 0);
  }
  
  void render(const gfx::SwapchainFrame *frame) {
    vkCmdBindPipeline(frame->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    renderQuad(frame->cmdBuffer);
  }
}




