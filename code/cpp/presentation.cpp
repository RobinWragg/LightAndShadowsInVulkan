#include "presentation.h"
#include "main.h"
#include "input.h"
#include "DrawCall.h"
#include "shadows.h"
#include "geometry.h"

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
  
  static mat4 createProjectionMatrix(uint32_t width, uint32_t height, float fieldOfView) {
    float aspectRatio = width / (float)height;
    mat4 proj = perspective(fieldOfView, aspectRatio, 0.1f, 100.0f);
    
    // Flip the Y axis because Vulkan shaders expect positive Y to point downwards
    return scale(proj, vec3(1, -1, 1));
  }

  void init(VkDescriptorSetLayout shadowMapSamplerDescSetLayout) {
    gfx::createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(matrices), &matricesBuffer, &matricesBufferMemory);
    gfx::createDescriptorSet(matricesBuffer, &matricesDescSet, &matricesDescSetLayout);
    
    vector<VkDescriptorSetLayout> descriptorSetLayouts = {
      shadows::getMatricesDescSetLayout(),
      matricesDescSetLayout,
      shadowMapSamplerDescSetLayout
    };
    pipelineLayout = gfx::createPipelineLayout(descriptorSetLayouts.data(), (int)descriptorSetLayouts.size(), sizeof(mat4));
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
  
  static void bindDescriptorSets(VkCommandBuffer cmdBuffer, vector<ShadowMap> *shadowMaps) {
    // Update uniform buffers
    gfx::setBufferMemory(matricesBufferMemory, sizeof(matrices), &matrices);
    
    // Bind
    VkDescriptorSet shadowsDescSet = shadows::getMatricesDescSet();
    VkDescriptorSet sets[] = {shadowsDescSet, matricesDescSet, (*shadowMaps)[0].samplerDescriptorSet};
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 3, sets, 0, nullptr);
  }
  
  void renderLightSource(VkCommandBuffer cmdBuffer) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, unlitPipeline);
    
    lightSource->modelMatrix = translate(glm::identity<mat4>(), shadows::getLightPos());
    lightSource->modelMatrix = scale(lightSource->modelMatrix, vec3(0.1, 0.1, 0.1));
    
    lightSource->addToCmdBuffer(cmdBuffer, pipelineLayout);
  }
  
  void render(VkCommandBuffer cmdBuffer, vector<ShadowMap> *shadowMaps) {
    bindDescriptorSets(cmdBuffer, shadowMaps);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, litPipeline);
    geometry::addGeometryToCommandBuffer(cmdBuffer, pipelineLayout);
    renderLightSource(cmdBuffer);
  }
}






