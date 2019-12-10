#include "main.h"

#include "GraphicsFoundation.h"
#include "GraphicsPipeline.h"
#include "DrawCall.h"
#include "input.h"

namespace graphics {
  GraphicsFoundation *foundation = nullptr;
  GraphicsPipeline *pipeline = nullptr;
  DrawCall *pyramid = nullptr;
  DrawCall *ground = nullptr;
  DrawCall *sphereAngularNormals = nullptr;
  DrawCall *sphereSmoothNormals = nullptr;
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
      
      // For a unit sphere centred on the origin, the vertex positions are identical to the normals.
      vector<vec3> normals;
      for (auto &vert : verts) normals.push_back(vert);
      
      return new DrawCall(pipeline, verts, normals);
    } else return new DrawCall(pipeline, verts);
  }

  void init(SDL_Window *window) {
    foundation = new GraphicsFoundation(window);
    pipeline = new GraphicsPipeline(foundation, true);
    
    cameraPosition.x = 0;
    cameraPosition.y = 2;
    cameraPosition.z = 7;
    
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
    
    sphereAngularNormals = newSphereDrawCall(8, false);
    sphereSmoothNormals = newSphereDrawCall(8, true);
    
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
    perFrameUniform.matrix = identity<mat4>();
    
    VkExtent2D extent = foundation->getSurfaceExtent();
    float aspect = extent.width / (float)extent.height;
    perFrameUniform.matrix = perspective(radians(50.0f), aspect, 0.1f, 100.0f);
    perFrameUniform.matrix = scale(perFrameUniform.matrix, vec3(1, -1, 1));
    
    perFrameUniform.matrix = rotate(perFrameUniform.matrix, cameraAngle.y, vec3(1.0f, 0.0f, 0.0f));
    perFrameUniform.matrix = rotate(perFrameUniform.matrix, cameraAngle.x, vec3(0.0f, 1.0f, 0.0f));
    
    perFrameUniform.matrix = translate(perFrameUniform.matrix, -cameraPosition);
    
    // DrawCallUniform pyramidUniform;
    // pyramidUniform.matrix = identity<mat4>();
    // pyramidUniform.matrix = rotate(pyramidUniform.matrix, (float)getTime(), vec3(0.0f, 1.0f, 0.0f));
    // pipeline->submit(pyramid, &pyramidUniform);
    
    DrawCallUniform sphereAngularUniform;
    sphereAngularUniform.matrix = identity<mat4>();
    sphereAngularUniform.matrix = translate(sphereAngularUniform.matrix, vec3(-1.0f, 1.0f, 0.0f));
    sphereAngularUniform.matrix = rotate(sphereAngularUniform.matrix, (float)getTime(), vec3(0.0f, 0.4f, 1.0f));
    pipeline->submit(sphereAngularNormals, &sphereAngularUniform);
    
    DrawCallUniform sphereSmoothUniform;
    sphereSmoothUniform.matrix = identity<mat4>();
    sphereSmoothUniform.matrix = translate(sphereSmoothUniform.matrix, vec3(1.0f, 1.0f, 0.0f));
    sphereSmoothUniform.matrix = rotate(sphereSmoothUniform.matrix, (float)getTime()*0.7f, vec3(0.0f, 0.5f, 0.8f));
    pipeline->submit(sphereSmoothNormals, &sphereSmoothUniform);
    
    DrawCallUniform groundUniform;
    groundUniform.matrix = identity<mat4>();
    pipeline->submit(ground, &groundUniform);
    pipeline->present(&perFrameUniform);
  }

  void destroy() {
    delete pyramid;
    delete ground;
    delete sphereAngularNormals;
    delete sphereSmoothNormals;
    delete pipeline;
    delete foundation;
  }
}



