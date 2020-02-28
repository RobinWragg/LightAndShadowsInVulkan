#include "shadowMapViewer.h"
#include "main.h"
#include "graphics.h"

namespace shadowMapViewer {
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline       pipeline       = VK_NULL_HANDLE;
  
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  
  VkDescriptorSetLayout matrixDescriptorSetLayout;
  vector<VkBuffer> matrixBuffers;
  vector<VkDeviceMemory> matrixBufferMemories;
  vector<VkDescriptorSet> matrixDescriptorSets;
  
  vector<ShadowMap> *shadowMaps;
  
  vector<vec3> vertices = {
    vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 1, 0),
    vec3(0, 1, 0), vec3(1, 0, 0), vec3(1, 1, 0)
  };
  
  void init(vector<ShadowMap> *shadowMaps_) {
    shadowMaps = shadowMaps_;
    
    // Create matrix handles
    matrixBuffers.resize(shadowMaps->size());
    matrixBufferMemories.resize(shadowMaps->size());
    matrixDescriptorSets.resize(shadowMaps->size());
    
    for (int i = 0; i < shadowMaps->size(); i++) {
      gfx::createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(mat4), &matrixBuffers[i], &matrixBufferMemories[i]);
      gfx::createDescriptorSet(matrixBuffers[i], &matrixDescriptorSets[i], &matrixDescriptorSetLayout);
    }
    
    // Create pipeline
    VkDescriptorSetLayout descSetLayouts[] = {matrixDescriptorSetLayout, (*shadowMaps)[0].samplerDescriptorSetLayout};
    pipelineLayout = gfx::createPipelineLayout(descSetLayouts, 2, 0);
    pipeline = gfx::createPipeline(pipelineLayout, gfx::getSurfaceExtent(), gfx::renderPass, VK_CULL_MODE_BACK_BIT, 1, "shadowMapViewer.vert.spv", "shadowMapViewer.frag.spv");
    
    gfx::createVec3Buffer(vertices, &vertexBuffer, &vertexBufferMemory);
  }
  
  void renderQuad(VkCommandBuffer cmdBuffer, int shadowMapIndex) {
    float size = 0.8;
    mat4 matrix = glm::identity<mat4>();
    VkExtent2D extent = gfx::getSurfaceExtent();
    float aspectRatio = extent.width / (float)extent.height;
    matrix = scale(matrix, vec3(1 / aspectRatio, 1, 1));
    matrix = translate(matrix, vec3(-aspectRatio, 1-size - size*shadowMapIndex, 0));
    matrix = scale(matrix, vec3(size, size, 1));
    
    gfx::setBufferMemory(matrixBufferMemories[shadowMapIndex], sizeof(matrix), &matrix);
    
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &matrixDescriptorSets[shadowMapIndex], 0, nullptr);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(*shadowMaps)[shadowMapIndex].samplerDescriptorSet, 0, nullptr);
    
    VkDeviceSize vertexBufferOffset = 0;
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
    
    vkCmdDraw(cmdBuffer, (int)vertices.size(), 1, 0, 0);
  }
  
  void render(const gfx::SwapchainFrame *frame) {
    vkCmdBindPipeline(frame->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    // for (int i = 0; i < SHADOWMAP_COUNT; i++) {
    //   renderQuad(frame->cmdBuffer, i);
    // }
    renderQuad(frame->cmdBuffer, 0);
  }
}




