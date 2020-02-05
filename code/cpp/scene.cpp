#include "main.h"
#include "input.h"
#include "DrawCall.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace scene {
  
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline       pipeline       = VK_NULL_HANDLE;
  
  VkImage shadowMap;
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
    VkAttachmentDescription attachment = gfx::createAttachmentDescription(format, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    
    VkAttachmentReference attachmentRef = {};
    attachmentRef.attachment = 0; // The first and only attachment
    attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &attachmentRef;
    
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDesc;
    
    VkSubpassDependency subpassDep = gfx::createSubpassDependency();
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDep;
    
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &attachment;
    
    auto result = vkCreateRenderPass(gfx::device, &renderPassInfo, nullptr, &shadowMapRenderPass);
    SDL_assert_release(result == VK_SUCCESS);
  }
  
  void createShadowMapResources() {
    const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    
    int imageWidth, imageHeight, componentsPerPixel;
    unsigned char *imageData = stbi_load("test.png", &imageWidth, &imageHeight, &componentsPerPixel, 4);
    SDL_assert_release(imageData != nullptr);
    SDL_assert_release(imageWidth == 128);
    SDL_assert_release(imageHeight == 128);
    
    uint32_t componentCount = 4 * imageWidth * imageHeight;
    float *imageFloatData = new float[componentCount];
    for (uint32_t i = 0; i < componentCount; i++) {
      imageFloatData[i] = imageData[i] / 255.0f;
    }
    
    stbi_image_free(imageData);
    
    gfx::createColorImage(imageWidth, imageHeight, &shadowMap, &shadowMapMemory);
    gfx::setImageMemoryRGBA(shadowMap, shadowMapMemory, imageWidth, imageHeight, imageFloatData);
    shadowMapView = gfx::createImageView(shadowMap, format, VK_IMAGE_ASPECT_COLOR_BIT);
    
    delete [] imageFloatData;
    
    createShadowMapRenderPass(format);
    shadowMapFramebuffer = gfx::createFramebuffer(shadowMapRenderPass, {shadowMapView}, imageWidth, imageHeight);
  }

  void init() {
    pipelineLayout = gfx::createPipelineLayout(nullptr, 0, sizeof(mat4) * 2);
    uint32_t vertexAttributeCount = 2;
    pipeline = gfx::createPipeline(pipelineLayout, gfx::renderPass, vertexAttributeCount, "scene.vert.spv", "scene.frag.spv");
    
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
  
  mat4 getUpdatedViewProjectionMatrix(float deltaTime) {
    cameraAngle += input::getViewAngleInput();
    
    // Get player input for walking and take into account the direction the player is facing
    vec2 lateralMovement = input::getMovementVector() * (deltaTime * 2);
    lateralMovement = rotate(lateralMovement, -cameraAngle.x);
    
    cameraPosition.x += lateralMovement.x;
    cameraPosition.z -= lateralMovement.y;
    
    mat4 viewProjectionMatrix = glm::identity<mat4>();
    
    // projection transformation
    VkExtent2D extent = gfx::getSurfaceExtent();
    float aspectRatio = extent.width / (float)extent.height;
    viewProjectionMatrix = perspective(radians(50.0f), aspectRatio, 0.1f, 100.0f);
    
    // view transformation
    viewProjectionMatrix = scale(viewProjectionMatrix, vec3(1, -1, 1));
    viewProjectionMatrix = rotate(viewProjectionMatrix, cameraAngle.y, vec3(1.0f, 0.0f, 0.0f));
    viewProjectionMatrix = rotate(viewProjectionMatrix, cameraAngle.x, vec3(0.0f, 1.0f, 0.0f));
    viewProjectionMatrix = translate(viewProjectionMatrix, -cameraPosition);
    
    return viewProjectionMatrix;
  }
  
  void addToCommandBuffer(VkCommandBuffer cmdBuffer, float deltaTime) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    mat4 viewProjectionMatrix =  getUpdatedViewProjectionMatrix(deltaTime);
    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(viewProjectionMatrix), &viewProjectionMatrix);
    
    pyramid->modelMatrix = glm::identity<mat4>();
    pyramid->modelMatrix = translate(pyramid->modelMatrix, vec3(0, 0, -2));
    pyramid->modelMatrix = rotate(pyramid->modelMatrix, (float)getTime(), vec3(0.0f, 1.0f, 0.0f));
    pyramid->addToCmdBuffer(cmdBuffer, pipelineLayout);
    
    sphere0->modelMatrix = glm::identity<mat4>();
    sphere0->modelMatrix = translate(sphere0->modelMatrix, vec3(-1.0f, 1.0f, 0.0f));
    sphere0->modelMatrix = rotate(sphere0->modelMatrix, (float)getTime(), vec3(0, 1, 0));
    sphere0->modelMatrix = scale(sphere0->modelMatrix, vec3(0.3, 1, 1));
    sphere0->addToCmdBuffer(cmdBuffer, pipelineLayout);
    
    sphere1->modelMatrix = glm::identity<mat4>();
    sphere1->modelMatrix = translate(sphere1->modelMatrix, vec3(1.0f, 1.0f, 0.0f));
    sphere1->modelMatrix = rotate(sphere1->modelMatrix, (float)getTime(), vec3(0, 0, 1));
    sphere1->modelMatrix = scale(sphere1->modelMatrix, vec3(0.3, 1, 1));
    sphere1->addToCmdBuffer(cmdBuffer, pipelineLayout);
    
    ground->modelMatrix = glm::identity<mat4>();
    ground->addToCmdBuffer(cmdBuffer, pipelineLayout);
  }
}



