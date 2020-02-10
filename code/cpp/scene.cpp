#include "scene.h"
#include "main.h"
#include "input.h"
#include "DrawCall.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace scene {
  
  VkPipelineLayout pipelineLayout    = VK_NULL_HANDLE;
  VkPipeline       shadowMapPipeline = VK_NULL_HANDLE;
  VkPipeline       litPipeline       = VK_NULL_HANDLE;
  VkPipeline       unlitPipeline     = VK_NULL_HANDLE;
  
  vector<VkDescriptorSet> descriptorSets;
  
  struct Pass {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    
    VkBuffer              viewMatrixBuffer        = VK_NULL_HANDLE;
    VkDeviceMemory        viewMatrixBufferMemory  = VK_NULL_HANDLE;
    VkDescriptorSet       viewMatrixDescSet       = VK_NULL_HANDLE;
    VkDescriptorSetLayout viewMatrixDescSetLayout = VK_NULL_HANDLE;
    
    VkBuffer              projectionMatrixBuffer        = VK_NULL_HANDLE;
    VkDeviceMemory        projectionMatrixBufferMemory  = VK_NULL_HANDLE;
    VkDescriptorSet       projectionMatrixDescSet       = VK_NULL_HANDLE;
    VkDescriptorSetLayout projectionMatrixDescSetLayout = VK_NULL_HANDLE;
  } shadowMapPass, presentationPass;
  
  ShadowMap *shadowMap;
  VkFramebuffer shadowMapFramebuffer;
  VkRenderPass shadowMapRenderPass;
  
  DrawCall *pyramid     = nullptr;
  DrawCall *ground      = nullptr;
  DrawCall *sphere0     = nullptr;
  DrawCall *sphere1     = nullptr;
  DrawCall *lightSource = nullptr;
  
  vec3 cameraPosition;
  vec2 cameraAngle;
  
  void addRingVertices(vec3 translation, int sideCount, float height, float btmRadius, float topRadius, vector<vec3> *verts) {
    
    const vec3 normal = vec3(0, 1, 0);
    
    for (int i = 0; i < sideCount; i++) {
      float angle0 = (i / (float)sideCount) * M_TAU;
      float angle1 = ((i+1) / (float)sideCount) * M_TAU;
      
      vec3 vert00 = rotate(vec3(btmRadius, 0, 0), angle0, normal);
      vec3 vert10 = rotate(vec3(btmRadius, 0, 0), angle1, normal);
      vec3 vert01 = rotate(vec3(topRadius, height, 0), angle0, normal);
      vec3 vert11 = rotate(vec3(topRadius, height, 0), angle1, normal);
      
      verts->push_back(vert10 + translation);
      verts->push_back(vert00 + translation);
      verts->push_back(vert01 + translation);
      
      verts->push_back(vert10 + translation);
      verts->push_back(vert01 + translation);
      verts->push_back(vert11 + translation);
    }
  }
  
  DrawCall * newSphereDrawCall(int resolution, bool smoothNormals) {
    vector<vec3> verts;
    
    for (int i = 0; i < resolution; i++) {
      float verticalAngle0 = (i / (float)resolution) * M_PI;
      float verticalAngle1 = ((i+1) / (float)resolution) * M_PI;
      
      float btmRadius = sinf(verticalAngle0);
      float topRadius = sinf(verticalAngle1);
      
      float btmY = -cosf(verticalAngle0);
      float topY = -cosf(verticalAngle1);
      
      addRingVertices(vec3(0, btmY, 0), resolution*2, topY - btmY, btmRadius, topRadius, &verts);
    }
    
    if (smoothNormals) {
      
      // For a unit sphere centered on the origin, the vertex positions are identical to the normals.
      vector<vec3> normals;
      for (auto &vert : verts) normals.push_back(vert);
      
      return new DrawCall(verts, normals);
    } else return new DrawCall(verts);
  }
  
  void createShadowMapRenderPass(VkFormat format) {
    VkAttachmentDescription colorAttachment = gfx::createAttachmentDescription(format, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
    
    auto result = vkCreateRenderPass(gfx::device, &renderPassInfo, nullptr, &shadowMapRenderPass);
    SDL_assert_release(result == VK_SUCCESS);
  }
  
  void createShadowMapResources() {
    createShadowMapRenderPass(shadowMap->format);
    shadowMapFramebuffer = gfx::createFramebuffer(shadowMapRenderPass, {shadowMap->imageView, gfx::depthImageView}, shadowMap->width, shadowMap->height);
    
    uint32_t vertexAttributeCount = 2;
    VkExtent2D extent;
    extent.width = shadowMap->width;
    extent.height = shadowMap->height;
    shadowMapPipeline = gfx::createPipeline(pipelineLayout, extent, shadowMapRenderPass, vertexAttributeCount, "shadowMap.vert.spv", "shadowMap.frag.spv");
  }
  
  void initDescriptorSetsForPass(Pass &pass) {
    gfx::createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(mat4), &pass.viewMatrixBuffer, &pass.viewMatrixBufferMemory);
    gfx::createDescriptorSet(pass.viewMatrixBuffer, &pass.viewMatrixDescSet, &pass.viewMatrixDescSetLayout);
    
    gfx::createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(mat4), &pass.projectionMatrixBuffer, &pass.projectionMatrixBufferMemory);
    gfx::createDescriptorSet(pass.projectionMatrixBuffer, &pass.projectionMatrixDescSet, &pass.projectionMatrixDescSetLayout);
  }
  
  static mat4 createProjectionMatrix(uint32_t width, uint32_t height, float fieldOfView) {
    float aspectRatio = width / (float)height;
    mat4 proj = perspective(fieldOfView, aspectRatio, 0.1f, 100.0f);
    
    // Flip the Y axis because Vulkan shaders expect positive Y to point downwards
    return scale(proj, vec3(1, -1, 1));
  }
  
  vector<vec3> createGroundVertices() {
    const float halfWidth = 3;
    const float height = 0.1;
    
    vector<vec3> vertices = {
      {-halfWidth, 0, -halfWidth}, {halfWidth, 0, -halfWidth}, {-halfWidth, 0, halfWidth},
      {-halfWidth, 0, halfWidth}, {halfWidth, 0, -halfWidth}, {halfWidth, 0, halfWidth},
      
      {-halfWidth, 0, -halfWidth}, {-halfWidth, -height, -halfWidth}, {halfWidth, 0, -halfWidth},
      {-halfWidth, -height, -halfWidth}, {halfWidth, -height, -halfWidth}, {halfWidth, 0, -halfWidth},
      
      {-halfWidth, 0, -halfWidth}, {-halfWidth, 0, halfWidth}, {-halfWidth, -height, -halfWidth},
      {-halfWidth, -height, -halfWidth}, {-halfWidth, 0, halfWidth}, {-halfWidth, -height, halfWidth},
      
      {-halfWidth, 0, halfWidth}, {halfWidth, 0, halfWidth}, {-halfWidth, -height, halfWidth},
      {-halfWidth, -height, halfWidth}, {halfWidth, 0, halfWidth}, {halfWidth, -height, halfWidth},
      
      {halfWidth, 0, -halfWidth}, {halfWidth, -height, -halfWidth}, {halfWidth, 0, halfWidth},
      {halfWidth, -height, -halfWidth}, {halfWidth, -height, halfWidth}, {halfWidth, 0, halfWidth},
    };
    
    return vertices;
  }

  void init(ShadowMap *shadowMap_) {
    shadowMap = shadowMap_;
    
    initDescriptorSetsForPass(shadowMapPass);
    initDescriptorSetsForPass(presentationPass);
    
    vector<VkDescriptorSetLayout> descriptorSetLayouts = {
      shadowMapPass.viewMatrixDescSetLayout,
      shadowMapPass.projectionMatrixDescSetLayout,
      presentationPass.viewMatrixDescSetLayout,
      presentationPass.projectionMatrixDescSetLayout,
      shadowMap->samplerDescriptorSetLayout
    };
    descriptorSets = {
      shadowMapPass.viewMatrixDescSet,
      shadowMapPass.projectionMatrixDescSet,
      presentationPass.viewMatrixDescSet,
      presentationPass.projectionMatrixDescSet,
      shadowMap->samplerDescriptorSet
    };
    pipelineLayout = gfx::createPipelineLayout(descriptorSetLayouts.data(), (int)descriptorSetLayouts.size(), sizeof(mat4));
    uint32_t vertexAttributeCount = 2;
    litPipeline = gfx::createPipeline(pipelineLayout, gfx::getSurfaceExtent(), gfx::renderPass, vertexAttributeCount, "lit.vert.spv", "lit.frag.spv");
    unlitPipeline = gfx::createPipeline(pipelineLayout, gfx::getSurfaceExtent(), gfx::renderPass, vertexAttributeCount, "unlit.vert.spv", "unlit.frag.spv");
    
    createShadowMapResources();
    
    VkExtent2D extent = gfx::getSurfaceExtent();
    presentationPass.projectionMatrix = createProjectionMatrix(extent.width, extent.height, 1.5);
    shadowMapPass.projectionMatrix = createProjectionMatrix(shadowMap->width, shadowMap->height, 1.5);
    
    cameraPosition.x = 4;
    cameraPosition.y = 2;
    cameraPosition.z = 6.2;
    
    cameraAngle.x = -0.57;
    cameraAngle.y = 0.27;
    
    vector<vec3> pyramidVertices = {
      {0, 0, 0}, {1, 0, 0}, {0, 1, 0},
      {0, 0, 0}, {0, 0, 1}, {1, 0, 0},
      {0, 0, 0}, {0, 1, 0}, {0, 0, 1},
      {1, 0, 0}, {0, 0, 1}, {0, 1, 0},
    };
    
    pyramid = new DrawCall(pyramidVertices);
    ground = new DrawCall(createGroundVertices());
    sphere0 = newSphereDrawCall(32, true);
    sphere1 = newSphereDrawCall(32, true);
    lightSource = newSphereDrawCall(16, true);
    
    printf("\nInitialised Vulkan\n");
  }
  
  static void updatePresentationViewMatrix(float deltaTime) {
    cameraAngle += input::getViewAngleInput();
    
    // Get player input for walking and take into account the direction the player is facing
    vec2 lateralMovement = input::getMovementVector() * (deltaTime * 2);
    lateralMovement = rotate(lateralMovement, -cameraAngle.x);
    
    cameraPosition.x += lateralMovement.x;
    cameraPosition.z -= lateralMovement.y;
    
    presentationPass.viewMatrix = glm::identity<mat4>();
    
    // view transformation
    presentationPass.viewMatrix = rotate(presentationPass.viewMatrix, cameraAngle.y, vec3(1.0f, 0.0f, 0.0f));
    presentationPass.viewMatrix = rotate(presentationPass.viewMatrix, cameraAngle.x, vec3(0.0f, 1.0f, 0.0f));
    presentationPass.viewMatrix = translate(presentationPass.viewMatrix, -cameraPosition);
  }
  
  static void updateShadowMapViewMatrix(float deltaTime) {
    shadowMapPass.viewMatrix = glm::identity<mat4>();
    shadowMapPass.viewMatrix = translate(shadowMapPass.viewMatrix, vec3(0, 0, -6));
    shadowMapPass.viewMatrix = rotate(shadowMapPass.viewMatrix, 0.7f, vec3(1.0f, 0.0f, 0.0f));
    shadowMapPass.viewMatrix = rotate(shadowMapPass.viewMatrix, (float)getTime()*1.3f, vec3(0.0f, 1.0f, 0.0f));
  }
  
  static void addDrawCallsToCommandBuffer(VkCommandBuffer cmdBuffer) {
    pyramid->addToCmdBuffer(cmdBuffer, pipelineLayout);
    sphere0->addToCmdBuffer(cmdBuffer, pipelineLayout);
    sphere1->addToCmdBuffer(cmdBuffer, pipelineLayout);
    ground->addToCmdBuffer(cmdBuffer, pipelineLayout);
  }
  
  void update(float deltaTime) {
    updatePresentationViewMatrix(deltaTime);
    updateShadowMapViewMatrix(deltaTime);
    
    pyramid->modelMatrix = glm::identity<mat4>();
    pyramid->modelMatrix = translate(pyramid->modelMatrix, vec3(0, 0, -2));
    
    sphere0->modelMatrix = glm::identity<mat4>();
    sphere0->modelMatrix = translate(sphere0->modelMatrix, vec3(-1.0f, 1.0f, 0.0f));
    sphere0->modelMatrix = rotate(sphere0->modelMatrix, (float)M_PI*1.5f, vec3(0, 1, 0));
    sphere0->modelMatrix = scale(sphere0->modelMatrix, vec3(0.3, 1, 1));
    
    sphere1->modelMatrix = glm::identity<mat4>();
    sphere1->modelMatrix = translate(sphere1->modelMatrix, vec3(1.0f, 1.0f, 0.0f));
    sphere1->modelMatrix = scale(sphere1->modelMatrix, vec3(0.3, 1, 1));
    
    ground->modelMatrix = glm::identity<mat4>();
    
    vec4 lightPos4 = inverse(shadowMapPass.viewMatrix) * vec4(0, 0, 0, 1);
    vec3 lightPos = vec3(lightPos4.x, lightPos4.y, lightPos4.z);
    lightSource->modelMatrix = translate(glm::identity<mat4>(), lightPos);
    lightSource->modelMatrix = scale(lightSource->modelMatrix, vec3(0.1, 0.1, 0.1));
  }
  
  static void bindDescriptorSets(VkCommandBuffer cmdBuffer) {
    // Update uniform buffers
    gfx::setBufferMemory(shadowMapPass.viewMatrixBufferMemory, sizeof(shadowMapPass.viewMatrix), &shadowMapPass.viewMatrix);
    gfx::setBufferMemory(shadowMapPass.projectionMatrixBufferMemory, sizeof(shadowMapPass.projectionMatrix), &shadowMapPass.projectionMatrix);
    gfx::setBufferMemory(presentationPass.viewMatrixBufferMemory, sizeof(presentationPass.viewMatrix), &presentationPass.viewMatrix);
    gfx::setBufferMemory(presentationPass.projectionMatrixBufferMemory, sizeof(presentationPass.projectionMatrix), &presentationPass.projectionMatrix);
    
    // Bind
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, (int)descriptorSets.size(), descriptorSets.data(), 0, nullptr);
  }
  
  void performShadowMapRenderPass(VkCommandBuffer cmdBuffer) {
    gfx::cmdBeginRenderPass(shadowMapRenderPass, shadowMap->width, shadowMap->height, shadowMapFramebuffer, cmdBuffer);
    
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipeline);
    
    bindDescriptorSets(cmdBuffer);
    addDrawCallsToCommandBuffer(cmdBuffer);
    
    vkCmdEndRenderPass(cmdBuffer);
  }
  
  void renderGeometry(VkCommandBuffer cmdBuffer) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, litPipeline);
    
    addDrawCallsToCommandBuffer(cmdBuffer);
  }
  
  void renderLightSource(VkCommandBuffer cmdBuffer) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, unlitPipeline);
    
    lightSource->addToCmdBuffer(cmdBuffer, pipelineLayout);
  }
  
  void render(VkCommandBuffer cmdBuffer) {
    bindDescriptorSets(cmdBuffer);
    renderGeometry(cmdBuffer);
    renderLightSource(cmdBuffer);
  }
}






