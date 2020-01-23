#include "main.h"

#include "GraphicsPipeline.h"
#include "DrawCall.h"
#include "input.h"

namespace graphics {
  GraphicsPipeline *pipeline = nullptr;
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
      
      return new DrawCall(pipeline, verts, normals);
    } else return new DrawCall(pipeline, verts);
  }

  void init(SDL_Window *window) {
    gfx::createCoreHandles(window);
    
    pipeline = new GraphicsPipeline(true);
    
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
    
    pyramid = new DrawCall(pipeline, pyramidVertices);
    
    vector<vec3> groundVertices = {
      {-3, 0, -3}, {3, 0, -3}, {-3, 0, 3},
      {-3, 0, 3}, {3, 0, -3}, {3, 0, 3}
    };
    
    ground = new DrawCall(pipeline, groundVertices);
    
    sphere0 = newSphereDrawCall(32, true);
    sphere1 = newSphereDrawCall(32, true);
    
    printf("\nInitialised Vulkan\n");
  }
  
  void updateAndRender(float dt) {
    cameraAngle += input::getViewAngleInput();
    
    // Get player input for walking and take into account the direction the player is facing
    vec2 lateralMovement = input::getMovementVector() * (dt * 2);
    lateralMovement = rotate(lateralMovement, -cameraAngle.x);
    
    cameraPosition.x += lateralMovement.x;
    cameraPosition.z -= lateralMovement.y;
    
    PerFrameUniform perFrameUniform;
    perFrameUniform.matrix = glm::identity<mat4>();
    
    VkExtent2D extent = gfx::getSurfaceExtent();
    float aspect = extent.width / (float)extent.height;
    perFrameUniform.matrix = perspective(radians(50.0f), aspect, 0.1f, 100.0f);
    perFrameUniform.matrix = scale(perFrameUniform.matrix, vec3(1, -1, 1));
    
    perFrameUniform.matrix = rotate(perFrameUniform.matrix, cameraAngle.y, vec3(1.0f, 0.0f, 0.0f));
    perFrameUniform.matrix = rotate(perFrameUniform.matrix, cameraAngle.x, vec3(0.0f, 1.0f, 0.0f));
    
    perFrameUniform.matrix = translate(perFrameUniform.matrix, -cameraPosition);
    
    DrawCallUniform pyramidUniform;
    pyramidUniform.matrix = glm::identity<mat4>();
    pyramidUniform.matrix = translate(pyramidUniform.matrix, vec3(0, 0, -2));
    pyramidUniform.matrix = rotate(pyramidUniform.matrix, (float)getTime(), vec3(0.0f, 1.0f, 0.0f));
    pipeline->submit(pyramid, &pyramidUniform);
    
    DrawCallUniform sphere0Uniform;
    sphere0Uniform.matrix = glm::identity<mat4>();
    sphere0Uniform.matrix = translate(sphere0Uniform.matrix, vec3(-1.0f, 1.0f, 0.0f));
    sphere0Uniform.matrix = rotate(sphere0Uniform.matrix, (float)getTime(), vec3(0, 1, 0));
    sphere0Uniform.matrix = scale(sphere0Uniform.matrix, vec3(0.3, 1, 1));
    pipeline->submit(sphere0, &sphere0Uniform);
    
    DrawCallUniform sphere1Uniform;
    sphere1Uniform.matrix = glm::identity<mat4>();
    sphere1Uniform.matrix = translate(sphere1Uniform.matrix, vec3(1.0f, 1.0f, 0.0f));
    sphere1Uniform.matrix = rotate(sphere1Uniform.matrix, (float)getTime(), vec3(0, 0, 1));
    sphere1Uniform.matrix = scale(sphere1Uniform.matrix, vec3(0.3, 1, 1));
    pipeline->submit(sphere1, &sphere1Uniform);
    
    DrawCallUniform groundUniform;
    groundUniform.matrix = glm::identity<mat4>();
    pipeline->submit(ground, &groundUniform);
    
    pipeline->present(&perFrameUniform);
  }

  void destroy() {
    delete pyramid;
    delete ground;
    delete sphere0;
    delete sphere1;
    delete pipeline;
  }
}



