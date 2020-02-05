#include "main.h"
#include "input.h"
#include "DrawCall.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace scene {
  
  VkPipelineLayout pipelineLayout    = VK_NULL_HANDLE;
  VkPipeline       shadowMapPipeline = VK_NULL_HANDLE;
  VkPipeline       scenePipeline     = VK_NULL_HANDLE;
  
  VkImage shadowMap;
  int shadowMapWidth, shadowMapHeight;
  VkImageView shadowMapView;
  VkDeviceMemory shadowMapMemory;
  VkFramebuffer shadowMapFramebuffer;
  VkRenderPass shadowMapRenderPass;
  
  DrawCall *pyramid = nullptr;
  DrawCall *ground  = nullptr;
  DrawCall *sphere0 = nullptr;
  DrawCall *sphere1 = nullptr;
  
  vec3 cameraPosition;
  vec2 cameraAngle;
  mat4 viewMatrix;
  
  VkImageView getShadowMapView() {
    return shadowMapView;
  }
  
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
    const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    
    shadowMapWidth = 512;
    shadowMapHeight = 512;
    
    gfx::createColorImage(shadowMapWidth, shadowMapHeight, &shadowMap, &shadowMapMemory);
    shadowMapView = gfx::createImageView(shadowMap, format, VK_IMAGE_ASPECT_COLOR_BIT);
    
    createShadowMapRenderPass(format);
    shadowMapFramebuffer = gfx::createFramebuffer(shadowMapRenderPass, {shadowMapView, gfx::depthImageView}, shadowMapWidth, shadowMapHeight);
    
    uint32_t vertexAttributeCount = 2;
    VkExtent2D extent;
    extent.width = shadowMapWidth;
    extent.height = shadowMapHeight;
    shadowMapPipeline = gfx::createPipeline(pipelineLayout, extent, shadowMapRenderPass, vertexAttributeCount, "shadowMap.vert.spv", "shadowMap.frag.spv");
  }

  void init() {
    pipelineLayout = gfx::createPipelineLayout(nullptr, 0, sizeof(mat4) * 2);
    uint32_t vertexAttributeCount = 2;
    scenePipeline = gfx::createPipeline(pipelineLayout, gfx::getSurfaceExtent(), gfx::renderPass, vertexAttributeCount, "scene.vert.spv", "scene.frag.spv");
    
    createShadowMapResources();
    
    cameraPosition.x = 0;
    cameraPosition.y = 2;
    cameraPosition.z = 5;
    
    cameraAngle.x = 0;
    cameraAngle.y = 0;
    
    vector<vec3> pyramidVertices = {
      {0, 0, 0}, {1, 0, 0}, {0, 1, 0},
      {0, 0, 0}, {0, 0, 1}, {1, 0, 0},
      {0, 0, 0}, {0, 1, 0}, {0, 0, 1},
      {1, 0, 0}, {0, 0, 1}, {0, 1, 0},
    };
    
    pyramid = new DrawCall(pyramidVertices);
    
    vector<vec3> groundVertices = {
      {-3, 0, -3}, {3, 0, -3}, {-3, 0, 3},
      {-3, 0, 3}, {3, 0, -3}, {3, 0, 3}
    };
    
    ground = new DrawCall(groundVertices);
    
    sphere0 = newSphereDrawCall(32, true);
    sphere1 = newSphereDrawCall(32, false);
    
    printf("\nInitialised Vulkan\n");
  }
  
  static mat4 createProjectionMatrix(uint32_t width, uint32_t height) {
    float aspectRatio = width / (float)height;
    return perspective(radians(50.0f), aspectRatio, 0.1f, 100.0f);
  }
  
  static void updateViewMatrix(float deltaTime) {
    cameraAngle += input::getViewAngleInput();
    
    // Get player input for walking and take into account the direction the player is facing
    vec2 lateralMovement = input::getMovementVector() * (deltaTime * 2);
    lateralMovement = rotate(lateralMovement, -cameraAngle.x);
    
    cameraPosition.x += lateralMovement.x;
    cameraPosition.z -= lateralMovement.y;
    
    viewMatrix = glm::identity<mat4>();
    
    // view transformation
    viewMatrix = scale(viewMatrix, vec3(1, -1, 1));
    viewMatrix = rotate(viewMatrix, cameraAngle.y, vec3(1.0f, 0.0f, 0.0f));
    viewMatrix = rotate(viewMatrix, cameraAngle.x, vec3(0.0f, 1.0f, 0.0f));
    viewMatrix = translate(viewMatrix, -cameraPosition);
  }
  
  void addDrawCallsToCommandBuffer(VkCommandBuffer cmdBuffer) {
    pyramid->addToCmdBuffer(cmdBuffer, pipelineLayout);
    sphere0->addToCmdBuffer(cmdBuffer, pipelineLayout);
    sphere1->addToCmdBuffer(cmdBuffer, pipelineLayout);
    ground->addToCmdBuffer(cmdBuffer, pipelineLayout);
  }
  
  void update(float deltaTime) {
    updateViewMatrix(deltaTime);
    
    pyramid->modelMatrix = glm::identity<mat4>();
    pyramid->modelMatrix = translate(pyramid->modelMatrix, vec3(0, 0, -2));
    pyramid->modelMatrix = rotate(pyramid->modelMatrix, (float)getTime(), vec3(0.0f, 1.0f, 0.0f));
    
    sphere0->modelMatrix = glm::identity<mat4>();
    sphere0->modelMatrix = translate(sphere0->modelMatrix, vec3(-1.0f, 1.0f, 0.0f));
    sphere0->modelMatrix = rotate(sphere0->modelMatrix, (float)getTime(), vec3(0, 1, 0));
    sphere0->modelMatrix = scale(sphere0->modelMatrix, vec3(0.3, 1, 1));
    
    sphere1->modelMatrix = glm::identity<mat4>();
    sphere1->modelMatrix = translate(sphere1->modelMatrix, vec3(1.0f, 1.0f, 0.0f));
    sphere1->modelMatrix = rotate(sphere1->modelMatrix, (float)getTime(), vec3(0, 0, 1));
    sphere1->modelMatrix = scale(sphere1->modelMatrix, vec3(0.3, 1, 1));
    
    ground->modelMatrix = glm::identity<mat4>();
  }
  
  void performShadowMapRenderPass(VkCommandBuffer cmdBuffer) {
    gfx::cmdBeginRenderPass(shadowMapRenderPass, shadowMapWidth, shadowMapHeight, shadowMapFramebuffer, cmdBuffer);
    
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipeline);
    
    mat4 viewProjectionMatrix = createProjectionMatrix(shadowMapWidth, shadowMapHeight) * viewMatrix;
    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(viewProjectionMatrix), &viewProjectionMatrix);
    
    addDrawCallsToCommandBuffer(cmdBuffer);
    
    vkCmdEndRenderPass(cmdBuffer);
    
    // VkImageMemoryBarrier barrier = {};
    // barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    // barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    // barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    // barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // barrier.image = shadowMap;
    
    // barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // barrier.subresourceRange.baseMipLevel = 0;
    // barrier.subresourceRange.levelCount = 1;
    // barrier.subresourceRange.baseArrayLayer = 0;
    // barrier.subresourceRange.layerCount = 1;
    
    // vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  }
  
  void renderScene(VkCommandBuffer cmdBuffer) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipeline);
    
    VkExtent2D extent = gfx::getSurfaceExtent();
    mat4 viewProjectionMatrix = createProjectionMatrix(extent.width, extent.height) * viewMatrix;
    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(viewProjectionMatrix), &viewProjectionMatrix);
    
    addDrawCallsToCommandBuffer(cmdBuffer);
  }
}



