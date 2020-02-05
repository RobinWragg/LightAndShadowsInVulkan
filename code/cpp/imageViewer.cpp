#include "imageViewer.h"
#include "main.h"
#include "graphics.h"
#include "scene.h"

namespace imageViewer {
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline       pipeline       = VK_NULL_HANDLE;
  
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  
  VkSampler shadowMapSampler;
  
  VkDescriptorSetLayout shadowMapSamplerDescriptorSetLayout;
  VkDescriptorSet shadowMapSamplerDescriptorSet;
  
  vector<vec3> vertices = {
    vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 1, 0),
    vec3(0, 1, 0), vec3(1, 0, 0), vec3(1, 1, 0)
  };
  
  void createShadowMapResources() {
    shadowMapSampler = gfx::createSampler();
    
    shadowMapSamplerDescriptorSetLayout = gfx::createDescriptorSetLayout(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    
    shadowMapSamplerDescriptorSet = gfx::createDescriptorSet(gfx::descriptorPool, shadowMapSamplerDescriptorSetLayout, scene::getShadowMapView(), shadowMapSampler);
  }
  
  void init() {
    createShadowMapResources();
    
    pipelineLayout = gfx::createPipelineLayout(&shadowMapSamplerDescriptorSetLayout, 1, sizeof(mat4));
    pipeline = gfx::createPipeline(pipelineLayout, gfx::getSurfaceExtent(), gfx::renderPass, 1, "imageViewer.vert.spv", "imageViewer.frag.spv");
    
    gfx::createVec3Buffer(vertices, &vertexBuffer, &vertexBufferMemory);
  }
  
  void renderQuad(VkCommandBuffer cmdBuffer) {
    mat4 matrix = glm::identity<mat4>();
    VkExtent2D extent = gfx::getSurfaceExtent();
    float aspectRatio = extent.width / (float)extent.height;
    matrix = scale(matrix, vec3(1 / aspectRatio, 1, 1));
    matrix = translate(matrix, vec3(-aspectRatio, 0, 0));
    // matrix = scale(matrix, vec3(0.5, 0.5, 1));
    
    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrix), &matrix);
    
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &shadowMapSamplerDescriptorSet, 0, nullptr);
    
    VkDeviceSize vertexBufferOffset = 0;
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
    
    vkCmdDraw(cmdBuffer, (int)vertices.size(), 1, 0, 0);
  }
  
  void addToCommandBuffer(const gfx::SwapchainFrame *frame) {
    vkCmdBindPipeline(frame->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    renderQuad(frame->cmdBuffer);
  }
}




