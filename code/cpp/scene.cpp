#include "scene.h"
#include "main.h"
#include "input.h"
#include "DrawCall.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace scene {
  
  VkPipelineLayout pipelineLayout    = VK_NULL_HANDLE;
  VkPipeline       shadowMapPipeline = VK_NULL_HANDLE;
  VkPipeline       litPipeline       = VK_NULL_HANDLE;
  VkPipeline       unlitPipeline     = VK_NULL_HANDLE;
  
  vector<VkDescriptorSet> descriptorSets;
  
  struct Pass {
    vec3 cameraPos;
    vec2 cameraAngle;
    
    struct {
      mat4 view;
      mat4 proj;
    } matrices;
    
    VkBuffer              matricesBuffer        = VK_NULL_HANDLE;
    VkDeviceMemory        matricesBufferMemory  = VK_NULL_HANDLE;
    VkDescriptorSet       matricesDescSet       = VK_NULL_HANDLE;
    VkDescriptorSetLayout matricesDescSetLayout = VK_NULL_HANDLE;
    
  } shadowMapPass, presentationPass;
  
  vector<ShadowMap> *shadowMaps;
  vector<VkFramebuffer> shadowMapFramebuffers;
  VkRenderPass shadowMapRenderPass;
  
  DrawCall *ground      = nullptr;
  DrawCall *sphere0     = nullptr;
  DrawCall *sphere1     = nullptr;
  DrawCall *sphere2     = nullptr;
  DrawCall *sphere3     = nullptr;
  DrawCall *aeroplane   = nullptr;
  DrawCall *frog        = nullptr;
  DrawCall *lightSource = nullptr;
  
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
    createShadowMapRenderPass((*shadowMaps)[0].format);
    
    shadowMapFramebuffers.resize(shadowMaps->size());
    for (int i = 0; i < shadowMaps->size(); i++) {
      ShadowMap &shadowMap = (*shadowMaps)[i];
      shadowMapFramebuffers[i] = gfx::createFramebuffer(shadowMapRenderPass, {shadowMap.imageView, shadowMap.depthImageView}, shadowMap.width, shadowMap.height);
    }
    
    uint32_t vertexAttributeCount = 2;
    VkExtent2D extent;
    extent.width = (*shadowMaps)[0].width;
    extent.height = (*shadowMaps)[0].height;
    shadowMapPipeline = gfx::createPipeline(pipelineLayout, extent, shadowMapRenderPass, VK_CULL_MODE_FRONT_BIT, vertexAttributeCount, "shadowMap.vert.spv", "shadowMap.frag.spv");
  }
  
  void initDescriptorSetsForPass(Pass &pass) {
    gfx::createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(pass.matrices), &pass.matricesBuffer, &pass.matricesBufferMemory);
    gfx::createDescriptorSet(pass.matricesBuffer, &pass.matricesDescSet, &pass.matricesDescSetLayout);    
  }
  
  static mat4 createProjectionMatrix(uint32_t width, uint32_t height, float fieldOfView) {
    float aspectRatio = width / (float)height;
    mat4 proj = perspective(fieldOfView, aspectRatio, 0.1f, 100.0f);
    
    // Flip the Y axis because Vulkan shaders expect positive Y to point downwards
    return scale(proj, vec3(1, -1, 1));
  }
  
  vector<vec3> createGroundVertices() {
    const float halfWidth = 6;
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
  
  DrawCall * newDrawCallFromObjFile(const char *filePath) {
    vector<vec3> vertices;
    vector<vec3> normals;
    
    tinyobj::attrib_t attributes;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;
    string warning;
    string error;
    bool ret = tinyobj::LoadObj(&attributes, &shapes, &materials, &warning, &error, filePath);
    printf("OBJ file errors: %s\n", error.c_str());
    printf("OBJ file warnings: %s\n", warning.c_str());
    SDL_assert_release(ret);
    
    // For each shape
    for (uint32_t s = 0; s < shapes.size(); s++) {
      uint32_t indexOffset = 0;
      
      // For each face
      for (uint32_t faceIndex = 0; faceIndex < shapes[s].mesh.num_face_vertices.size(); faceIndex++) {
        int faceVertCount = shapes[s].mesh.num_face_vertices[faceIndex];
        SDL_assert_release(faceVertCount == 3);
        
        // For each vertex
        for (uint32_t vertIndex = 0; vertIndex < faceVertCount; vertIndex++) {
          tinyobj::index_t idx = shapes[s].mesh.indices[indexOffset + vertIndex];
          
          vec3 vertex = vec3(
            attributes.vertices[3*idx.vertex_index],
            attributes.vertices[3*idx.vertex_index+1],
            attributes.vertices[3*idx.vertex_index+2]
            );
          vertices.push_back(vertex);
          
          vec3 normal = vec3(
            attributes.normals[3*idx.normal_index],
            attributes.normals[3*idx.normal_index+1],
            attributes.normals[3*idx.normal_index+2]
            );
          normals.push_back(normal);
          
          // vec2 texCoord = vec2(
          //   attributes.texcoords[2*idx.texcoord_index],
          //   attributes.texcoords[2*idx.texcoord_index+1]
          //   );
        }
        
        indexOffset += faceVertCount;
      }
    }
    
    // Convert to clockwise front faces
    for (uint32_t i = 0; i < vertices.size(); i += 3) {
      vec3 temp = vertices[i];
      vertices[i] = vertices[i+1];
      vertices[i+1] = temp;
      
      temp = normals[i];
      normals[i] = normals[i+1];
      normals[i+1] = temp;
    }
    
    SDL_assert_release(vertices.size() == normals.size());
    
    return new DrawCall(vertices, normals);
  }

  void init(vector<ShadowMap> *shadowMaps_) {
    shadowMaps = shadowMaps_;
    
    aeroplane = newDrawCallFromObjFile("aeroplane.obj");
    frog = newDrawCallFromObjFile("frog.obj");
    
    initDescriptorSetsForPass(shadowMapPass);
    initDescriptorSetsForPass(presentationPass);
    
    vector<VkDescriptorSetLayout> descriptorSetLayouts = {
      shadowMapPass.matricesDescSetLayout,
      presentationPass.matricesDescSetLayout,
      (*shadowMaps)[0].samplerDescriptorSetLayout
    };
    descriptorSets = {
      shadowMapPass.matricesDescSet,
      presentationPass.matricesDescSet,
      (*shadowMaps)[0].samplerDescriptorSet
    };
    pipelineLayout = gfx::createPipelineLayout(descriptorSetLayouts.data(), (int)descriptorSetLayouts.size(), sizeof(mat4));
    uint32_t vertexAttributeCount = 2;
    litPipeline = gfx::createPipeline(pipelineLayout, gfx::getSurfaceExtent(), gfx::renderPass, VK_CULL_MODE_BACK_BIT, vertexAttributeCount, "lit.vert.spv", "lit.frag.spv");
    unlitPipeline = gfx::createPipeline(pipelineLayout, gfx::getSurfaceExtent(), gfx::renderPass, VK_CULL_MODE_BACK_BIT, vertexAttributeCount, "unlit.vert.spv", "unlit.frag.spv");
    
    createShadowMapResources();
    
    VkExtent2D extent = gfx::getSurfaceExtent();
    presentationPass.matrices.proj = createProjectionMatrix(extent.width, extent.height, 0.8);
    shadowMapPass.matrices.proj = createProjectionMatrix((*shadowMaps)[0].width, (*shadowMaps)[0].height, 2);
    
    presentationPass.cameraPos.x = 3.388;
    presentationPass.cameraPos.y = 2;
    presentationPass.cameraPos.z = 1.294;
    
    presentationPass.cameraAngle.x = -0.858;
    presentationPass.cameraAngle.y = 0.698;
    
    ground = new DrawCall(createGroundVertices());
    sphere0 = newSphereDrawCall(2, true);
    sphere1 = newSphereDrawCall(3, true);
    sphere2 = newSphereDrawCall(8, true);
    sphere3 = newSphereDrawCall(64, true);
    lightSource = newSphereDrawCall(16, true);
    
    printf("\nInitialised Vulkan\n");
  }
  
  static void updatePresentationViewMatrix(float deltaTime) {
    presentationPass.cameraAngle += input::getViewAngleInput();
    
    // Get player input for walking and take into account the direction the player is facing
    vec2 lateralMovement = input::getMovementVector() * (deltaTime * 2);
    lateralMovement = rotate(lateralMovement, -presentationPass.cameraAngle.x);
    
    presentationPass.cameraPos.x += lateralMovement.x;
    presentationPass.cameraPos.z -= lateralMovement.y;
    
    presentationPass.matrices.view = glm::identity<mat4>();
    
    // view transformation
    presentationPass.matrices.view = rotate(presentationPass.matrices.view, presentationPass.cameraAngle.y, vec3(1.0f, 0.0f, 0.0f));
    presentationPass.matrices.view = rotate(presentationPass.matrices.view, presentationPass.cameraAngle.x, vec3(0.0f, 1.0f, 0.0f));
    presentationPass.matrices.view = translate(presentationPass.matrices.view, -presentationPass.cameraPos);
  }
  
  static void updateShadowMapViewMatrix() {
    
    // Camera/light positioning settings
    const float lateralDistanceFromOrigin = 9;
    const float minHeight = 4;
    const float maxHeight = 20;
    const float heightChangeSpeed = 0.4;
    const float lateralAngle = 2.5 + getTime()*0.5;
    
    float currentHeight = minHeight + (sinf(getTime() * heightChangeSpeed) + 1) * 0.5 * (maxHeight - minHeight);
    
    // Set cameraPos
    shadowMapPass.cameraPos.x = sinf(lateralAngle) * lateralDistanceFromOrigin;
    shadowMapPass.cameraPos.y = currentHeight;
    shadowMapPass.cameraPos.z = cosf(lateralAngle) * lateralDistanceFromOrigin;
        
    // Set cameraAngle; point the camera at the origin of the scene
    
    shadowMapPass.cameraAngle.x = -lateralAngle;
    
    // sin(theta) = opposite / hypotenuse
    // Therefore: theta = asin(opposite / hypotenuse)
    // float hypotenuse = hypot(lateralDistanceFromOrigin, shadowMapPass.cameraPos.y);
    shadowMapPass.cameraAngle.y = asinf(shadowMapPass.cameraPos.y / length(shadowMapPass.cameraPos));
    
    // Set the view matrix according to cameraPos and cameraAngle
    shadowMapPass.matrices.view = rotate(glm::identity<mat4>(), shadowMapPass.cameraAngle.y, vec3(1.0f, 0.0f, 0.0f));
    shadowMapPass.matrices.view = rotate(shadowMapPass.matrices.view, shadowMapPass.cameraAngle.x, vec3(0.0f, 1.0f, 0.0f));
    shadowMapPass.matrices.view = translate(shadowMapPass.matrices.view, -shadowMapPass.cameraPos);
  }
  
  static void addDrawCallsToCommandBuffer(VkCommandBuffer cmdBuffer) {
    sphere0->addToCmdBuffer(cmdBuffer, pipelineLayout);
    sphere1->addToCmdBuffer(cmdBuffer, pipelineLayout);
    sphere2->addToCmdBuffer(cmdBuffer, pipelineLayout);
    sphere3->addToCmdBuffer(cmdBuffer, pipelineLayout);
    aeroplane->addToCmdBuffer(cmdBuffer, pipelineLayout);
    frog->addToCmdBuffer(cmdBuffer, pipelineLayout);
    ground->addToCmdBuffer(cmdBuffer, pipelineLayout);
  }
  
  void update(float deltaTime) {
    updatePresentationViewMatrix(deltaTime);
    updateShadowMapViewMatrix();
    
    float sphereScale = 0.7;
    sphere0->modelMatrix = translate(glm::identity<mat4>(), vec3(-2.5, sphereScale, -3.5));
    sphere0->modelMatrix = scale(sphere0->modelMatrix, vec3(sphereScale, sphereScale, sphereScale));
    
    sphere1->modelMatrix = translate(glm::identity<mat4>(), vec3(-0.8333, sphereScale, -3.5));
    sphere1->modelMatrix = scale(sphere1->modelMatrix, vec3(sphereScale, sphereScale, sphereScale));
    
    sphere2->modelMatrix = translate(glm::identity<mat4>(), vec3(0.8333, sphereScale, -3.5));
    sphere2->modelMatrix = scale(sphere2->modelMatrix, vec3(sphereScale, sphereScale, sphereScale));
    
    sphere3->modelMatrix = translate(glm::identity<mat4>(), vec3(2.5, sphereScale, -3.5));
    sphere3->modelMatrix = scale(sphere3->modelMatrix, vec3(sphereScale, sphereScale, sphereScale));
    
    float aeroplaneScale = 0.6;
    aeroplane->modelMatrix = translate(glm::identity<mat4>(), vec3(2, 1.6, 2));
    aeroplane->modelMatrix = scale(aeroplane->modelMatrix, vec3(aeroplaneScale, aeroplaneScale, aeroplaneScale));
    aeroplane->modelMatrix = rotate(aeroplane->modelMatrix, 1.0f, vec3(0, 1, 0));
    aeroplane->modelMatrix = rotate(aeroplane->modelMatrix, 0.035f, vec3(1, 0, 0));
    
    float frogScale = 1;
    frog->modelMatrix = translate(glm::identity<mat4>(), vec3(2, 0.35, 4));
    frog->modelMatrix = scale(frog->modelMatrix, vec3(frogScale, frogScale, frogScale));
    frog->modelMatrix = rotate(frog->modelMatrix, -1.5f, vec3(0, 1, 0));
    frog->modelMatrix = rotate(frog->modelMatrix, -0.1f, vec3(1, 0, 0)); // even out the frog's feet
    
    ground->modelMatrix = glm::identity<mat4>();
    
    vec4 lightPos4 = inverse(shadowMapPass.matrices.view) * vec4(0, 0, 0, 1);
    vec3 lightPos = vec3(lightPos4.x, lightPos4.y, lightPos4.z);
    lightSource->modelMatrix = translate(glm::identity<mat4>(), lightPos);
    lightSource->modelMatrix = scale(lightSource->modelMatrix, vec3(0.1, 0.1, 0.1));
  }
  
  static void bindDescriptorSets(VkCommandBuffer cmdBuffer) {
    // Update uniform buffers
    gfx::setBufferMemory(shadowMapPass.matricesBufferMemory, sizeof(shadowMapPass.matrices), &shadowMapPass.matrices);
    gfx::setBufferMemory(presentationPass.matricesBufferMemory, sizeof(presentationPass.matrices), &presentationPass.matrices);
    
    // Bind
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, (int)descriptorSets.size(), descriptorSets.data(), 0, nullptr);
  }
  
  void performShadowMapRenderPasses(VkCommandBuffer cmdBuffer) {
    // This clear color must be higher than all rendered distances. The INFINITY macro cannot be used as it causes buggy rasterisation behaviour; GLSL doesn't officially support the IEEE infinity constant.
    vec3 clearColor = {1000, 1000, 1000};
    
    for (int i = 0; i < SHADOWMAP_COUNT; i++) {
      ShadowMap &shadowMap = (*shadowMaps)[i];
      gfx::cmdBeginRenderPass(shadowMapRenderPass, shadowMap.width, shadowMap.height, clearColor, shadowMapFramebuffers[i], cmdBuffer);
      
      vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipeline);
      
      bindDescriptorSets(cmdBuffer);
      addDrawCallsToCommandBuffer(cmdBuffer);
      
      vkCmdEndRenderPass(cmdBuffer);
    }
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






