#include "presentation.h"
#include "main.h"
#include "input.h"
#include "DrawCall.h"
#include "shadows.h"
#include "geometry.h"
#include "settings.h"

namespace presentation {
  
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline       litPipeline    = VK_NULL_HANDLE;
  VkPipeline       unlitPipeline  = VK_NULL_HANDLE;
  
  vector<VkDescriptorSet> descriptorSets;
  
  vec3 cameraPos;
  vec2 cameraAngle;
  
  DrawCall *lightSource = nullptr;
  
  struct {
    mat4 view;
    mat4 proj;
  } matrices;
  
  VkBuffer              matricesBuffer        = VK_NULL_HANDLE;
  VkDeviceMemory        matricesBufferMemory  = VK_NULL_HANDLE;
  VkDescriptorSet       matricesDescSet       = VK_NULL_HANDLE;
  VkDescriptorSetLayout matricesDescSetLayout = VK_NULL_HANDLE;
  
  VkBuffer              lightViewOffsetsBuffer        = VK_NULL_HANDLE;
  VkDeviceMemory        lightViewOffsetsBufferMemory  = VK_NULL_HANDLE;
  VkDescriptorSet       lightViewOffsetsDescSet       = VK_NULL_HANDLE;
  VkDescriptorSetLayout lightViewOffsetsDescSetLayout = VK_NULL_HANDLE;
  
  static mat4 createProjectionMatrix(uint32_t width, uint32_t height, float fieldOfView) {
    float aspectRatio = width / (float)height;
    mat4 proj = perspective(fieldOfView, aspectRatio, 0.1f, 100.0f);
    
    // Flip the Y axis because Vulkan shaders expect positive Y to point downwards
    return scale(proj, vec3(1, -1, 1));
  }

  void init(VkDescriptorSetLayout shadowMapSamplerDescSetLayout) {
    gfx::createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(matrices), &matricesBuffer, &matricesBufferMemory);
    gfx::createDescriptorSet(matricesBuffer, &matricesDescSet, &matricesDescSetLayout);
    
    gfx::createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(vec2) * MAX_SUBSOURCE_COUNT, &lightViewOffsetsBuffer, &lightViewOffsetsBufferMemory);
    gfx::createDescriptorSet(lightViewOffsetsBuffer, &lightViewOffsetsDescSet, &lightViewOffsetsDescSetLayout);
    
    vector<VkDescriptorSetLayout> descriptorSetLayouts = {
      DrawCall::worldMatrixDescSetLayout,
      shadows::getMatricesDescSetLayout(),
      matricesDescSetLayout,
      lightViewOffsetsDescSetLayout
    };
    for (int i = 0; i < MAX_SUBSOURCE_COUNT; i++) descriptorSetLayouts.push_back(shadowMapSamplerDescSetLayout);
      
    pipelineLayout = gfx::createPipelineLayout(descriptorSetLayouts.data(), (int)descriptorSetLayouts.size(), sizeof(int32_t));
    uint32_t vertexAttributeCount = 2;
    litPipeline = gfx::createPipeline(pipelineLayout, gfx::getSurfaceExtent(), gfx::renderPass, VK_CULL_MODE_BACK_BIT, vertexAttributeCount, "lit.vert.spv", "lit.frag.spv");
    unlitPipeline = gfx::createPipeline(pipelineLayout, gfx::getSurfaceExtent(), gfx::renderPass, VK_CULL_MODE_BACK_BIT, vertexAttributeCount, "unlit.vert.spv", "unlit.frag.spv");
    
    VkExtent2D extent = gfx::getSurfaceExtent();
    matrices.proj = createProjectionMatrix(extent.width, extent.height, 0.8);
    
    cameraPos.x = 3.388;
    cameraPos.y = 2;
    cameraPos.z = 1.294;
    
    cameraAngle.x = -0.858;
    cameraAngle.y = 0.698;
    
    lightSource = geometry::newSphereDrawCall(16, true);
  }
  
  static void updateViewMatrix(float deltaTime) {
    cameraAngle += input::getViewAngleInput();
    
    // Get player input for walking and take into account the direction the player is facing
    vec2 lateralMovement = input::getMovementVector() * (deltaTime * 2);
    lateralMovement = rotate(lateralMovement, -cameraAngle.x);
    
    cameraPos.x += lateralMovement.x;
    cameraPos.z -= lateralMovement.y;
    
    matrices.view = glm::identity<mat4>();
    
    // view transformation
    matrices.view = rotate(matrices.view, cameraAngle.y, vec3(1.0f, 0.0f, 0.0f));
    matrices.view = rotate(matrices.view, cameraAngle.x, vec3(0.0f, 1.0f, 0.0f));
    matrices.view = translate(matrices.view, -cameraPos);
  }
  
  void update(float deltaTime) {
    updateViewMatrix(deltaTime);
  }
  
  static void setUniforms(VkCommandBuffer cmdBuffer, vector<ShadowMap> *shadowMaps) {
    // Update matrices buffer
    gfx::setBufferMemory(matricesBufferMemory, sizeof(matrices), &matrices);
    
    // Update light view offset buffer
    auto lightViewOffsets = shadows::getViewOffsets();
    gfx::setBufferMemory(lightViewOffsetsBufferMemory, sizeof(lightViewOffsets[0]) * lightViewOffsets.size(), lightViewOffsets.data());
    
    // Bind
    VkDescriptorSet lightMatricesDescSet = shadows::getMatricesDescSet();
    vector<VkDescriptorSet> sets = {lightMatricesDescSet, matricesDescSet, lightViewOffsetsDescSet};
    for (int i = 0; i < MAX_SUBSOURCE_COUNT; i++) sets.push_back((*shadowMaps)[i].samplerDescriptorSet);
    
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, (int)sets.size(), sets.data(), 0, nullptr);
    
    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(int32_t), &settings.subsourceCount);
  }
  
  void renderLightSource(VkCommandBuffer cmdBuffer) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, unlitPipeline);
    
    lightSource->worldMatrix = translate(glm::identity<mat4>(), shadows::getLightPos());
    lightSource->worldMatrix = scale(lightSource->worldMatrix, vec3(0.1, 0.1, 0.1));
    
    lightSource->addToCmdBuffer(cmdBuffer, pipelineLayout);
  }
  
  void render(VkCommandBuffer cmdBuffer, vector<ShadowMap> *shadowMaps) {
    setUniforms(cmdBuffer, shadowMaps);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, litPipeline);
    geometry::addGeometryToCommandBuffer(cmdBuffer, pipelineLayout);
    renderLightSource(cmdBuffer);
  }
}






