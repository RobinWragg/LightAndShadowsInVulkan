#include "shadows.h"
#include "geometry.h"
#include "settings.h"

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
  
  VkBuffer        matricesBuffer       = VK_NULL_HANDLE;
  VkDeviceMemory  matricesBufferMemory = VK_NULL_HANDLE;
  VkDescriptorSet matricesDescSet      = VK_NULL_HANDLE;
  
  void createRenderPass() {
    VkAttachmentDescription colorAttachment = gfx::createAttachmentDescription((*shadowMaps)[0].format, true, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkAttachmentDescription depthAttachment = gfx::createAttachmentDescription(gfx::depthImageFormat, true, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    
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
    matricesDescSet = gfx::createDescSet(matricesBuffer);
    
    createRenderPass();
    
    framebuffers.resize(shadowMaps->size());
    
    for (int i = 0; i < shadowMaps->size(); i++) {
      ShadowMap &shadowMap = (*shadowMaps)[i];
      framebuffers[i] = gfx::createFramebuffer(renderPass, {shadowMap.imageView, shadowMap.depthImageView}, shadowMap.width, shadowMap.height);
    }
    
    vector<VkDescriptorSetLayout> descSetLayouts = {gfx::bufferDescLayout, gfx::bufferDescLayout};
    pipelineLayout = gfx::createPipelineLayout(descSetLayouts.data(), (int)descSetLayouts.size(), sizeof(vec2));
    
    vector<VkFormat> vertAttribFormats = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT};
    VkExtent2D extent;
    extent.width = (*shadowMaps)[0].width;
    extent.height = (*shadowMaps)[0].height;
    pipeline = gfx::createPipeline(pipelineLayout, vertAttribFormats, extent, renderPass, VK_CULL_MODE_FRONT_BIT, "shadowMap.vert.spv", "shadowMap.frag.spv");
  }
  
  void update() {
    lightPos.y = 5;
    
    if (settings.animateLightPos) {
      float angle = getTime() * 0.2;
      lightPos.x = cosf(angle) * 7;
      lightPos.z = sinf(angle) * 7;
    } else {
      lightPos.x = 0;
      lightPos.z = 0.0001; // Non-zero in order to work around a bug in glm::lookAt()
    }
    
    lightPos = vec3(-7, 15, -18);
    
    matrices.view = lookAt(lightPos, vec3(0, 0, 0), vec3(0, 1, 0));
    
    float fieldOfView = 2.3;
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
    
    int offsetCount = settings.subsourceCount;
    
    if (offsetCount == 1) return {vec2(0, 0)};
    
    bool ringMode = settings.subsourceArrangement == settings.RING;
    
    if (ringMode) {
      vec2 unit(0, settings.sourceRadius);
      
      for (int i = 0; i < offsetCount; i++) {
        offsets.push_back(rotate(unit, float(i/float(offsetCount) * M_TAU)));
      }
    } else {
      // Spiral mode
      for (int i = 0; i < offsetCount; i++) {
        float originDistance = float(i+1)/offsetCount;
        offsets.push_back(vec2(0, settings.sourceRadius * originDistance));
        offsets[i] = rotate(offsets[i], float(M_TAU*1.6 * (offsetCount-i)/offsetCount));
      }
    }
    
    return offsets;
  }
  
  void performRenderPasses(VkCommandBuffer cmdBuffer) {
    // This clear color must be higher than all rendered distances. The INFINITY macro cannot be used as it causes buggy rasterisation behaviour; GLSL doesn't officially support the IEEE infinity constant.
    vec3 clearColor = {1000, 1000, 1000};
    auto viewOffsets = getViewOffsets();
    
    // Execute a renderpass for all possible shadowmaps even if they're not used, in order to convert their layouts.
    for (int i = 0; i < MAX_LIGHT_SUBSOURCE_COUNT; i++) {
      ShadowMap &shadowMap = (*shadowMaps)[i];
      gfx::cmdBeginRenderPass(renderPass, shadowMap.width, shadowMap.height, clearColor, framebuffers[i], cmdBuffer);
      
      // Only render the shadowmaps that are being used this frame
      if (i < settings.subsourceCount) {
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        
        auto updatedMatricesDescSet = getMatricesDescSet();
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &updatedMatricesDescSet, 0, nullptr);
        
        vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(vec2), &viewOffsets[i]);
        
        geometry::renderAllGeometryWithoutSamplers(cmdBuffer, pipelineLayout);
      }
      
      vkCmdEndRenderPass(cmdBuffer);
    }
  }
  
  vec3 getLightPos() {
    return lightPos;
  }
}






