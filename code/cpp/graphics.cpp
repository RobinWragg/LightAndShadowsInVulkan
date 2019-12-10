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
  vec3 cameraPosition;
  vec2 cameraAngle;

  void init(SDL_Window *window) {
    foundation = new GraphicsFoundation(window);
    pipeline = new GraphicsPipeline(foundation, true);
    
    cameraPosition.x = 0;
    cameraPosition.y = 1;
    cameraPosition.z = 3;
    
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
    perFrameUniform.matrix = perspective(radians(50.0f), aspect, 0.1f, 10.0f);
    perFrameUniform.matrix = scale(perFrameUniform.matrix, vec3(1, -1, 1));
    
    perFrameUniform.matrix = rotate(perFrameUniform.matrix, cameraAngle.y, vec3(1.0f, 0.0f, 0.0f));
    perFrameUniform.matrix = rotate(perFrameUniform.matrix, cameraAngle.x, vec3(0.0f, 1.0f, 0.0f));
    
    perFrameUniform.matrix = translate(perFrameUniform.matrix, -cameraPosition);
    
    pipeline->submit(pyramid);
    pipeline->submit(ground);
    pipeline->present(&perFrameUniform);
  }

  void destroy() {
    delete pyramid;
    delete ground;
    delete pipeline;
    delete foundation;
  }
}



