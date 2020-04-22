#include "geometry.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace geometry {
  
  DrawCall *floor    = nullptr;
  vector<DrawCall*> spheres(3);
  DrawCall *sphereDiffuse = nullptr;
  DrawCall *aeroplane = nullptr;
  DrawCall *frog      = nullptr;
  
  VkDescriptorSet floorSamplerDescSet;
  VkDescriptorSet floorNormalSamplerDescSet;
  
  VkDescriptorSet frogSamplerDescSet;
  
  VkDescriptorSet aeroplaneSamplerDescSet;
  
  DrawCall * newDrawCallFromObjFile(const char *filePath) {
    vector<vec3> vertices;
    vector<vec3> normals;
    vector<vec2> texCoords;
    
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
          
          vec2 texCoord = vec2(
            attributes.texcoords[2*idx.texcoord_index],
            attributes.texcoords[2*idx.texcoord_index+1]
            );
          texCoords.push_back(texCoord);
        }
        
        indexOffset += faceVertCount;
      }
    }
    
    SDL_assert_release(vertices.size() == normals.size());
    
    return new DrawCall(vertices, normals, texCoords);
  }
  
  vector<vec3> createCuboidVertices(float width, float height, float yOffset) {
    const float halfWidth = width / 2;
    
    vector<vec3> vertices = {
      // Top
      {-halfWidth, yOffset+height, -halfWidth}, {-halfWidth, yOffset+height, halfWidth}, {halfWidth, yOffset+height, -halfWidth},
      {-halfWidth, yOffset+height, halfWidth}, {halfWidth, yOffset+height, halfWidth}, {halfWidth, yOffset+height, -halfWidth},
      
      // Bottom
      {-halfWidth, yOffset, -halfWidth}, {halfWidth, yOffset, -halfWidth}, {-halfWidth, yOffset, halfWidth},
      {-halfWidth, yOffset, halfWidth}, {halfWidth, yOffset, -halfWidth}, {halfWidth, yOffset, halfWidth},
      
      // Sides
      {-halfWidth, yOffset+height, -halfWidth}, {halfWidth, yOffset+height, -halfWidth}, {-halfWidth, yOffset, -halfWidth},
      {-halfWidth, yOffset, -halfWidth}, {halfWidth, yOffset+height, -halfWidth}, {halfWidth, yOffset, -halfWidth},
      
      {-halfWidth, yOffset+height, -halfWidth}, {-halfWidth, yOffset, -halfWidth}, {-halfWidth, yOffset+height, halfWidth},
      {-halfWidth, yOffset, -halfWidth}, {-halfWidth, yOffset, halfWidth}, {-halfWidth, yOffset+height, halfWidth},
      
      {-halfWidth, yOffset+height, halfWidth}, {-halfWidth, yOffset, halfWidth}, {halfWidth, yOffset+height, halfWidth},
      {-halfWidth, yOffset, halfWidth}, {halfWidth, yOffset, halfWidth}, {halfWidth, yOffset+height, halfWidth},
      
      {halfWidth, yOffset+height, -halfWidth}, {halfWidth, yOffset+height, halfWidth}, {halfWidth, yOffset, -halfWidth},
      {halfWidth, yOffset, -halfWidth}, {halfWidth, yOffset+height, halfWidth}, {halfWidth, yOffset, halfWidth},
    };
    
    return vertices;
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
      
      verts->push_back(vert00 + translation);
      verts->push_back(vert10 + translation);
      verts->push_back(vert01 + translation);
      
      verts->push_back(vert01 + translation);
      verts->push_back(vert10 + translation);
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
  
  void createFloor() {
    auto positions = createCuboidVertices(12, 0.5, -0.5);
    
    vector<vec2> texCoords = {
      {0, 0}, {0, 1}, {1, 0},
      {0, 1}, {1, 1}, {1, 0},
    };
    
    while (texCoords.size() < positions.size()) {
      texCoords.push_back(vec2(0, 0));
    }
    
    floor = new DrawCall(positions, {}, texCoords);
  }
  
  void init() {
    aeroplane = newDrawCallFromObjFile("aeroplane.obj");
    frog = newDrawCallFromObjFile("frog.obj");
    
    createFloor();
    
    float sphereScale = 1.0;
    for (int i = 0; i < spheres.size(); i++) {
      spheres[i] = newSphereDrawCall(64, true);
      
      float posX = (i - (spheres.size()-1)/2.0f) * 3.5;
      vec3 position(posX, sphereScale, -3.5);
      spheres[i]->descSetData.worldMatrix = translate(glm::identity<mat4>(), position);
      
      spheres[i]->descSetData.worldMatrix = scale(spheres[i]->descSetData.worldMatrix, vec3(sphereScale));
    }
    
    spheres[0]->descSetData.diffuseReflectionConst = 0.8;
    spheres[0]->descSetData.specReflectionConst = 0;
    spheres[0]->descSetData.specPowerConst = 1;
    
    spheres[1]->descSetData.diffuseReflectionConst = 0.4;
    spheres[1]->descSetData.specReflectionConst = 0.4;
    spheres[1]->descSetData.specPowerConst = 10;
    
    spheres[2]->descSetData.diffuseReflectionConst = 0;
    spheres[2]->descSetData.specReflectionConst = 0.8;
    spheres[2]->descSetData.specPowerConst = 30;
        
    float aeroplaneScale = 0.6;
    aeroplane->descSetData.worldMatrix = translate(glm::identity<mat4>(), vec3(3, 1.6, 2));
    aeroplane->descSetData.worldMatrix = scale(aeroplane->descSetData.worldMatrix, vec3(aeroplaneScale, aeroplaneScale, aeroplaneScale));
    aeroplane->descSetData.worldMatrix = rotate(aeroplane->descSetData.worldMatrix, 0.2f, vec3(0, 1, 0));
    aeroplane->descSetData.worldMatrix = rotate(aeroplane->descSetData.worldMatrix, 0.035f, vec3(1, 0, 0));
    
    float frogScale = 1;
    frog->descSetData.worldMatrix = translate(glm::identity<mat4>(), vec3(2, 0.35, 4));
    frog->descSetData.worldMatrix = scale(frog->descSetData.worldMatrix, vec3(frogScale, frogScale, frogScale));
    frog->descSetData.worldMatrix = rotate(frog->descSetData.worldMatrix, -1.5f, vec3(0, 1, 0));
    frog->descSetData.worldMatrix = rotate(frog->descSetData.worldMatrix, -0.1f, vec3(1, 0, 0)); // even out the frog's feet
    
    floor->descSetData.worldMatrix = glm::identity<mat4>();
    
    // Create floor texture sampler
    {
      VkImage image;
      VkDeviceMemory imageMemory;
      VkImageView imageView;
      gfx::loadImage("floorboards.jpg", false, &image, &imageMemory, &imageView);
      VkSampler sampler = gfx::createSampler();
      floorSamplerDescSet = gfx::createDescSet(imageView, sampler);
    }
    
    // Create floor normalmap sampler
    {
      VkImage image;
      VkDeviceMemory imageMemory;
      VkImageView imageView;
      gfx::loadImage("floorboards_normals.jpg", true, &image, &imageMemory, &imageView);
      VkSampler sampler = gfx::createSampler();
      floorNormalSamplerDescSet = gfx::createDescSet(imageView, sampler);
    }
    
    // Create frog texture sampler
    {
      VkImage image;
      VkDeviceMemory imageMemory;
      VkImageView imageView;
      gfx::loadImage("frog_texture.jpg", false, &image, &imageMemory, &imageView);
      VkSampler sampler = gfx::createSampler();
      frogSamplerDescSet = gfx::createDescSet(imageView, sampler);
    }
    
    // Create aeroplane texture sampler
    {
      VkImage image;
      VkDeviceMemory imageMemory;
      VkImageView imageView;
      gfx::loadImage("aeroplane.jpg", false, &image, &imageMemory, &imageView);
      VkSampler sampler = gfx::createSampler();
      aeroplaneSamplerDescSet = gfx::createDescSet(imageView, sampler);
    }
  }
  
  void renderAllGeometryWithoutSamplers(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout) {
    for (auto &sphere : spheres) sphere->addToCmdBuffer(cmdBuffer, pipelineLayout);
    frog->addToCmdBuffer(cmdBuffer, pipelineLayout);
    aeroplane->addToCmdBuffer(cmdBuffer, pipelineLayout);
    floor->addToCmdBuffer(cmdBuffer, pipelineLayout);
  }
  
  void renderBareGeometry(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout) {
    for (auto &sphere : spheres) sphere->addToCmdBuffer(cmdBuffer, pipelineLayout);
  }
  
  void renderTexturedGeometry(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout) {
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 18, 1, &frogSamplerDescSet, 0, nullptr);
    frog->addToCmdBuffer(cmdBuffer, pipelineLayout);
    
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 18, 1, &aeroplaneSamplerDescSet, 0, nullptr);
    aeroplane->addToCmdBuffer(cmdBuffer, pipelineLayout);
  }
  
  void renderTexturedNormalMappedGeometry(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout) {
    vector<VkDescriptorSet> floorDescSets = {floorSamplerDescSet, floorNormalSamplerDescSet};
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 18, (int)floorDescSets.size(), floorDescSets.data(), 0, nullptr);
    
    floor->addToCmdBuffer(cmdBuffer, pipelineLayout);
  }
}






