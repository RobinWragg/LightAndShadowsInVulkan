#include "shadows.h"
#include "geometry.h"

namespace shadows {
  VkRenderPass renderPass;
  VkPipeline       pipeline       = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  
  vector<ShadowMap> *shadowMaps;
  vector<VkFramebuffer> framebuffers;
  
  vec3 lightPos;
  vec2 lightAngle;
  
  struct {
    mat4 view;
    mat4 proj;
  } matrices;
  
  VkBuffer              matricesBuffer        = VK_NULL_HANDLE;
  VkDeviceMemory        matricesBufferMemory  = VK_NULL_HANDLE;
  VkDescriptorSet       matricesDescSet       = VK_NULL_HANDLE;
  VkDescriptorSetLayout matricesDescSetLayout = VK_NULL_HANDLE;
  
  void createRenderPass() {
    VkAttachmentDescription colorAttachment = gfx::createAttachmentDescription((*shadowMaps)[0].format, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkAttachmentDescription depthAttachment = gfx::createAttachmentDescription(VK_FORMAT_D32_SFLOAT, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorAttachmentRef;
    subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;
    
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDesc;
    
    VkSubpassDependency subpassDep = gfx::createSubpassDependency();
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDep;
    
    vector<VkAttachmentDescription> attachments = { colorAttachment, depthAttachment };
    renderPassInfo.attachmentCount = (uint32_t)attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    
    auto result = vkCreateRenderPass(gfx::device, &renderPassInfo, nullptr, &renderPass);
    SDL_assert_release(result == VK_SUCCESS);
  }
  
  void init(vector<ShadowMap> *shadowMaps_) {
    shadowMaps = shadowMaps_;
    
    gfx::createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(matrices), &matricesBuffer, &matricesBufferMemory);
    gfx::createDescriptorSet(matricesBuffer, &matricesDescSet, &matricesDescSetLayout);
    
    createRenderPass();
    
    framebuffers.resize(shadowMaps->size());
    
    for (int i = 0; i < shadowMaps->size(); i++) {
      ShadowMap &shadowMap = (*shadowMaps)[i];
      framebuffers[i] = gfx::createFramebuffer(renderPass, {shadowMap.imageView, shadowMap.depthImageView}, shadowMap.width, shadowMap.height);
    }
    
    vector<VkDescriptorSetLayout> descSetLayouts = {matricesDescSetLayout, DrawCall::worldMatrixDescSetLayout};
    pipelineLayout = gfx::createPipelineLayout(descSetLayouts.data(), (int)descSetLayouts.size(), sizeof(vec2));
    
    const uint32_t vertexAttributeCount = 2;
    VkExtent2D extent;
    extent.width = (*shadowMaps)[0].width;
    extent.height = (*shadowMaps)[0].height;
    pipeline = gfx::createPipeline(pipelineLayout, extent, renderPass, VK_CULL_MODE_FRONT_BIT, vertexAttributeCount, "shadowMap.vert.spv", "shadowMap.frag.spv");
  }
  
  VkDescriptorSetLayout getMatricesDescSetLayout() {
    return matricesDescSetLayout;
  }
  
  void update() {
    // Camera/light positioning settings
    const float lateralDistanceFromOrigin = 9;
    const float minHeight = 4;
    const float maxHeight = 10;
    const float heightChangeSpeed = 0.4;
    const float lateralAngle = 2.5 + getTime()*0.5;
    
    float currentHeight = minHeight + (sinf(getTime() * heightChangeSpeed) + 1) * 0.5 * (maxHeight - minHeight);
    
    // Set lightPos
    lightPos.x = sinf(lateralAngle) * lateralDistanceFromOrigin;
    lightPos.y = currentHeight;
    lightPos.z = cosf(lateralAngle) * lateralDistanceFromOrigin;
        
    // Set lightAngle; point the camera at the origin of the scene
    lightAngle.x = -lateralAngle;
    
    // sin(theta) = opposite / hypotenuse
    // Therefore: theta = asin(opposite / hypotenuse)
    // float hypotenuse = hypot(lateralDistanceFromOrigin, lightPos.y);
    lightAngle.y = asinf(lightPos.y / length(lightPos));
    
    // Set the view matrix according to lightPos and lightAngle
    matrices.view = rotate(glm::identity<mat4>(), lightAngle.y, vec3(1.0f, 0.0f, 0.0f));
    matrices.view = rotate(matrices.view, lightAngle.x, vec3(0.0f, 1.0f, 0.0f));
    matrices.view = translate(matrices.view, -lightPos);
    
    float fieldOfView = 20.0f / length(lightPos);
    float aspectRatio = (*shadowMaps)[0].width / (float)(*shadowMaps)[0].height;
    matrices.proj = perspective(fieldOfView, aspectRatio, 0.1f, 100.0f);
    
    // Flip the Y axis because Vulkan shaders expect positive Y to point downwards
    matrices.proj = scale(matrices.proj, vec3(1, -1, 1));
  }
  
  VkDescriptorSet getMatricesDescSet() {
    gfx::setBufferMemory(matricesBufferMemory, sizeof(matrices), &matrices);
    SDL_assert_release(matricesDescSet != VK_NULL_HANDLE);
    return matricesDescSet;
  }
  
  vector<vec2> getViewOffsets() {
    vector<vec2> offsets;
    
    if (shadowMapCount == 1) return {vec2(0, 0)};
    
    bool ringMode = false;
    float radius = 0.5;
    
    if (ringMode) {
      vec2 unit(0, radius);
      
      for (int i = 0; i < shadowMapCount; i++) {
        offsets.push_back(rotate(unit, float(i/float(shadowMapCount) * M_TAU)));
      }
    } else {
      // Spiral mode
      for (int i = 0; i < shadowMapCount; i++) {
        float originDistance = float(i+1)/shadowMapCount;
        offsets.push_back(vec2(0, radius * originDistance));
        offsets[i] = rotate(offsets[i], float(M_TAU*3.1 * (shadowMapCount-i)/shadowMapCount));
      }
    }
    
    return offsets;
  }
  
  void performRenderPasses(VkCommandBuffer cmdBuffer) {
    // This clear color must be higher than all rendered distances. The INFINITY macro cannot be used as it causes buggy rasterisation behaviour; GLSL doesn't officially support the IEEE infinity constant.
    vec3 clearColor = {1000, 1000, 1000};
    auto viewOffsets = getViewOffsets();
    
    // Execute a renderpass for all possible shadowmaps even if they're not used, in order to convert their layouts.
    for (int i = 0; i < MAX_SHADOWMAP_COUNT; i++) {
      ShadowMap &shadowMap = (*shadowMaps)[i];
      gfx::cmdBeginRenderPass(renderPass, shadowMap.width, shadowMap.height, clearColor, framebuffers[i], cmdBuffer);
      
      // Only render the shadowmaps that are being used this frame
      if (i < shadowMapCount) {
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        
        auto updatedMatricesDescSet = getMatricesDescSet();
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &updatedMatricesDescSet, 0, nullptr);
        
        vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(vec2), &viewOffsets[i]);
        
        geometry::addGeometryToCommandBuffer(cmdBuffer, pipelineLayout);
      }
      
      vkCmdEndRenderPass(cmdBuffer);
    }
  }
  
  vec3 getLightPos() {
    return lightPos;
  }
}





