#include "main.h"

#include "DrawCall.h"
#include "input.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace scene {
  
  VkPipelineLayout pipelineLayout;
  VkPipeline       pipeline;
  VkSemaphore      imageAvailableSemaphore;
  VkSemaphore      renderCompletedSemaphore;
  
  DrawCall *pyramid = nullptr;
  DrawCall *ground = nullptr;
  DrawCall *sphere0 = nullptr;
  DrawCall *sphere1 = nullptr;
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
  
  void createSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    SDL_assert_release(vkCreateSemaphore(gfx::device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) == VK_SUCCESS);
    SDL_assert_release(vkCreateSemaphore(gfx::device, &semaphoreInfo, nullptr, &renderCompletedSemaphore) == VK_SUCCESS);
  }
  
  void beginCommandBuffer(const gfx::SwapchainFrame *frame, const mat4 &viewProjectionMatrix) {
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = gfx::renderPass;
    renderPassInfo.framebuffer = frame->framebuffer;
    
    vector<VkClearValue> clearValues(2);
    
    clearValues[0].color.float32[0] = 0.5;
    clearValues[0].color.float32[1] = 0.7;
    clearValues[0].color.float32[2] = 1;
    clearValues[0].color.float32[3] = 1;
    
    clearValues[1].depthStencil.depth = 1;
    clearValues[1].depthStencil.stencil = 0;
    
    renderPassInfo.clearValueCount = (int)clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = gfx::getSurfaceExtent();
    
    gfx::beginCommandBuffer(frame->cmdBuffer);
    
    vkCmdBeginRenderPass(frame->cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(frame->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    vkCmdPushConstants(frame->cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(viewProjectionMatrix), &viewProjectionMatrix);
  }
  
  void endCommandBuffer(const gfx::SwapchainFrame *frame) {
    vkCmdEndRenderPass(frame->cmdBuffer);

    auto result = vkEndCommandBuffer(frame->cmdBuffer);
    SDL_assert(result == VK_SUCCESS);
  }

  void init(SDL_Window *window) {
    gfx::createCoreHandles(window);
    
    pipelineLayout = gfx::createPipelineLayout(nullptr, 0);
    pipeline = gfx::createPipeline(pipelineLayout, gfx::renderPass);
    
    createSemaphores();
    
    int imageWidth, imageHeight, componentsPerPixel;
    unsigned char *imageData = stbi_load("test.png", &imageWidth, &imageHeight, &componentsPerPixel, 4);
    SDL_assert_release(imageData != nullptr);
    SDL_assert_release(imageWidth == 128);
    SDL_assert_release(imageHeight == 128);
    
    uint32_t componentCount = 4 * imageWidth * imageHeight;
    float *imageFloatData = new float[componentCount];
    for (uint32_t i = 0; i < imageWidth*imageHeight; i++) {
      imageFloatData[i] = imageData[i] / 255.0f;
    }
    
    stbi_image_free(imageData);
    
    VkImage testImage;
    VkDeviceMemory testImageMemory;
    
    gfx::createColorImage(imageWidth, imageHeight, &testImage, &testImageMemory);
    
    gfx::setImageMemoryRGBA(testImage, testImageMemory, imageWidth, imageHeight, imageFloatData);
    
    delete [] imageFloatData;
    
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
    sphere1 = newSphereDrawCall(32, true);
    
    printf("\nInitialised Vulkan\n");
  }
  
  mat4 getUpdatedViewProjectionMatrix(float dt) {
    cameraAngle += input::getViewAngleInput();
    
    // Get player input for walking and take into account the direction the player is facing
    vec2 lateralMovement = input::getMovementVector() * (dt * 2);
    lateralMovement = rotate(lateralMovement, -cameraAngle.x);
    
    cameraPosition.x += lateralMovement.x;
    cameraPosition.z -= lateralMovement.y;
    
    mat4 viewProjectionMatrix = glm::identity<mat4>();
    
    // projection transformation
    VkExtent2D extent = gfx::getSurfaceExtent();
    float aspect = extent.width / (float)extent.height;
    viewProjectionMatrix = perspective(radians(50.0f), aspect, 0.1f, 100.0f);
    
    // view transformation
    viewProjectionMatrix = scale(viewProjectionMatrix, vec3(1, -1, 1));
    viewProjectionMatrix = rotate(viewProjectionMatrix, cameraAngle.y, vec3(1.0f, 0.0f, 0.0f));
    viewProjectionMatrix = rotate(viewProjectionMatrix, cameraAngle.x, vec3(0.0f, 1.0f, 0.0f));
    viewProjectionMatrix = translate(viewProjectionMatrix, -cameraPosition);
    
    return viewProjectionMatrix;
  }
  
  void fillCommandBuffer(const gfx::SwapchainFrame *frame, const mat4 &viewProjectionMatrix) {
    beginCommandBuffer(frame, viewProjectionMatrix);
    
    pyramid->modelMatrix = glm::identity<mat4>();
    pyramid->modelMatrix = translate(pyramid->modelMatrix, vec3(0, 0, -2));
    pyramid->modelMatrix = rotate(pyramid->modelMatrix, (float)getTime(), vec3(0.0f, 1.0f, 0.0f));
    pyramid->addToCmdBuffer(frame->cmdBuffer, pipelineLayout);
    
    sphere0->modelMatrix = glm::identity<mat4>();
    sphere0->modelMatrix = translate(sphere0->modelMatrix, vec3(-1.0f, 1.0f, 0.0f));
    sphere0->modelMatrix = rotate(sphere0->modelMatrix, (float)getTime(), vec3(0, 1, 0));
    sphere0->modelMatrix = scale(sphere0->modelMatrix, vec3(0.3, 1, 1));
    sphere0->addToCmdBuffer(frame->cmdBuffer, pipelineLayout);
    
    sphere1->modelMatrix = glm::identity<mat4>();
    sphere1->modelMatrix = translate(sphere1->modelMatrix, vec3(1.0f, 1.0f, 0.0f));
    sphere1->modelMatrix = rotate(sphere1->modelMatrix, (float)getTime(), vec3(0, 0, 1));
    sphere1->modelMatrix = scale(sphere1->modelMatrix, vec3(0.3, 1, 1));
    sphere1->addToCmdBuffer(frame->cmdBuffer, pipelineLayout);
    
    ground->modelMatrix = glm::identity<mat4>();
    ground->addToCmdBuffer(frame->cmdBuffer, pipelineLayout);
    
    endCommandBuffer(frame);
  }
  
  void updateAndRender(float dt) {
    gfx::SwapchainFrame *frame = gfx::getNextFrame(imageAvailableSemaphore);
    
    // Wait for the command buffer to finish executing
    vkWaitForFences(gfx::device, 1, &frame->cmdBufferFence, VK_TRUE, INT64_MAX);
    vkResetFences(gfx::device, 1, &frame->cmdBufferFence);
    
    fillCommandBuffer(frame, getUpdatedViewProjectionMatrix(dt));
    
    // Submit the command buffer
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    gfx::submitCommandBuffer(frame->cmdBuffer, imageAvailableSemaphore, waitStage, renderCompletedSemaphore, frame->cmdBufferFence);
    
    gfx::presentFrame(frame, renderCompletedSemaphore);
  }
}



