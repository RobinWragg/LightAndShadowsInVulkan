#include "presentation.h"
#include "main.h"
#include "input.h"
#include "DrawCall.h"
#include "shadows.h"
#include "geometry.h"
#include "settings.h"

namespace presentation {
  
  VkPipelineLayout basicPipelineLayout    = VK_NULL_HANDLE;
  VkPipelineLayout texturedPipelineLayout = VK_NULL_HANDLE;
  VkPipeline       litPipeline            = VK_NULL_HANDLE;
  VkPipeline       litTexturedPipeline    = VK_NULL_HANDLE;
  VkPipeline       unlitPipeline          = VK_NULL_HANDLE;
  
  vec3 cameraPos;
  vec2 cameraAngle;
  
  DrawCall *lightSource = nullptr;
  
  struct {
    mat4 view;
    mat4 proj;
  } matrices;
  
  struct PushConstants {
    int32_t subsourceCount;
    int32_t shadowAntiAliasSize;
    uint32_t renderTexturesBool;
    uint32_t renderNormalMapsBool;
    float ambReflection;
  } pushConstants;
  
  VkBuffer              matricesBuffer        = VK_NULL_HANDLE;
  VkDeviceMemory        matricesBufferMemory  = VK_NULL_HANDLE;
  VkDescriptorSet       matricesDescSet       = VK_NULL_HANDLE;
  
  VkBuffer              lightViewOffsetsBuffer        = VK_NULL_HANDLE;
  VkDeviceMemory        lightViewOffsetsBufferMemory  = VK_NULL_HANDLE;
  VkDescriptorSet       lightViewOffsetsDescSet       = VK_NULL_HANDLE;
  
  static mat4 createProjectionMatrix(uint32_t width, uint32_t height, float fieldOfView) {
    float aspectRatio = width / (float)height;
    mat4 proj = perspective(fieldOfView, aspectRatio, 0.1f, 100.0f);
    
    // Flip the Y axis because Vulkan shaders expect positive Y to point downwards
    return scale(proj, vec3(1, -1, 1));
  }

  void init() {
    gfx::createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(matrices), &matricesBuffer, &matricesBufferMemory);
    matricesDescSet = gfx::createDescSet(matricesBuffer);
    gfx::createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(vec2) * MAX_LIGHT_SUBSOURCE_COUNT, &lightViewOffsetsBuffer, &lightViewOffsetsBufferMemory);
    lightViewOffsetsDescSet = gfx::createDescSet(lightViewOffsetsBuffer);
    
    vector<VkDescriptorSetLayout> descriptorSetLayouts = {
      gfx::bufferDescLayout, // drawcall world matrix
      gfx::bufferDescLayout, // shadow matrices
      gfx::bufferDescLayout, // camera matrices
      gfx::bufferDescLayout, // light view offsets
    };
    
    // Add layouts for shadowmaps
    for (int i = 0; i < MAX_LIGHT_SUBSOURCE_COUNT; i++) {
      descriptorSetLayouts.push_back(gfx::samplerDescLayout);
    }
    
    basicPipelineLayout = gfx::createPipelineLayout(descriptorSetLayouts.data(), (int)descriptorSetLayouts.size(), sizeof(PushConstants));
    vector<VkFormat> vertAttribFormats = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT};
    litPipeline = gfx::createPipeline(basicPipelineLayout, vertAttribFormats, gfx::getSurfaceExtent(), gfx::renderPass, VK_CULL_MODE_BACK_BIT, "lit.vert.spv", "lit.frag.spv", MSAA_SETTING);
    unlitPipeline = gfx::createPipeline(basicPipelineLayout, vertAttribFormats, gfx::getSurfaceExtent(), gfx::renderPass, VK_CULL_MODE_BACK_BIT, "unlit.vert.spv", "unlit.frag.spv", MSAA_SETTING);
    
    {
      vector<VkDescriptorSetLayout> descriptorSetLayouts = {
        gfx::bufferDescLayout, // drawcall world matrix
        gfx::bufferDescLayout, // shadow matrices
        gfx::bufferDescLayout, // camera matrices
        gfx::bufferDescLayout, // light view offsets
      };
      
      // Add layouts for shadowmaps
      for (int i = 0; i < MAX_LIGHT_SUBSOURCE_COUNT; i++) {
        descriptorSetLayouts.push_back(gfx::samplerDescLayout);
      }
      
      // Add texture sampler layout
      descriptorSetLayouts.push_back(gfx::samplerDescLayout);
      
      // Add normalmap sampler layout
      descriptorSetLayouts.push_back(gfx::samplerDescLayout);
      
      texturedPipelineLayout = gfx::createPipelineLayout(descriptorSetLayouts.data(), (int)descriptorSetLayouts.size(), sizeof(PushConstants));
      vector<VkFormat> vertAttribFormats = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT};
      litTexturedPipeline = gfx::createPipeline(texturedPipelineLayout, vertAttribFormats, gfx::getSurfaceExtent(), gfx::renderPass, VK_CULL_MODE_BACK_BIT, "litTextured.vert.spv", "litTextured.frag.spv", MSAA_SETTING);
    }
    
    VkExtent2D extent = gfx::getSurfaceExtent();
    matrices.proj = createProjectionMatrix(extent.width, extent.height, 0.6);
    
    cameraPos.x = 3.388;
    cameraPos.y = 2;
    cameraPos.z = 1.294;
    
    cameraAngle.x = -0.858;
    cameraAngle.y = 0.698;
    
    lightSource = geometry::newSphereDrawCall(16, true);
  }
  
  static void updateViewMatrix(float deltaTime, bool firstPersonMode) {
    
    if (firstPersonMode) {
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
    } else {
      // Camera positioning settings
      const float lateralDistanceFromOrigin = 20;
      const float lateralAngle = 3.5;
      // const float lateralAngle = 2.5 + getTime()*0.1;
      
      float height = 8;
      
      // Set cameraPos
      cameraPos.x = sinf(lateralAngle) * lateralDistanceFromOrigin;
      cameraPos.y = height;
      cameraPos.z = cosf(lateralAngle) * lateralDistanceFromOrigin;
      
      matrices.view = lookAt(cameraPos, vec3(0, 1, 0), vec3(0, 1, 0));
    }
  }
  
  void update(float deltaTime) {
    updateViewMatrix(deltaTime, false);
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
    for (int i = 0; i < MAX_LIGHT_SUBSOURCE_COUNT; i++) sets.push_back((*shadowMaps)[i].samplerDescriptorSet);
    
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, basicPipelineLayout, 1, (int)sets.size(), sets.data(), 0, nullptr);
    
    pushConstants.subsourceCount       = settings.subsourceCount;
    pushConstants.shadowAntiAliasSize  = settings.shadowAntiAliasSize;
    pushConstants.renderTexturesBool   = settings.renderTextures;
    pushConstants.renderNormalMapsBool = settings.renderNormalMaps;
    pushConstants.ambReflection        = settings.ambReflection;
    vkCmdPushConstants(cmdBuffer, basicPipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(pushConstants), &pushConstants);
  }
  
  void renderLightSource(VkCommandBuffer cmdBuffer) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, unlitPipeline);
    
    lightSource->descSetData.worldMatrix = translate(glm::identity<mat4>(), shadows::getLightPos());
    lightSource->descSetData.worldMatrix = scale(lightSource->descSetData.worldMatrix, vec3(0.2, 0.2, 0.2));
    
    lightSource->addToCmdBuffer(cmdBuffer, basicPipelineLayout);
  }
  
  void render(VkCommandBuffer cmdBuffer, vector<ShadowMap> *shadowMaps) {
    setUniforms(cmdBuffer, shadowMaps);
    
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, litPipeline);
    geometry::renderBareGeometry(cmdBuffer, basicPipelineLayout);
    
    if (settings.renderTextures || settings.renderNormalMaps) {
      vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, litTexturedPipeline);
    }
    
    geometry::renderTexturedNormalMappedGeometry(cmdBuffer, texturedPipelineLayout);
    
    pushConstants.renderNormalMapsBool = 0;
    vkCmdPushConstants(cmdBuffer, basicPipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(pushConstants), &pushConstants);
    geometry::renderTexturedGeometry(cmdBuffer, texturedPipelineLayout);
    
    renderLightSource(cmdBuffer);
  }
}






