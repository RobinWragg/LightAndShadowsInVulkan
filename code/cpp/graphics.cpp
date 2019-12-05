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
    
    pyramid = new DrawCall(foundation, pyramidVertices);
    
    vector<vec3> groundVertices = {
      {-3, 0, -3}, {3, 0, -3}, {-3, 0, 3},
      {-3, 0, 3}, {3, 0, -3}, {3, 0, 3}
    };
    
    ground = new DrawCall(foundation, groundVertices);
    
    printf("\nInitialised Vulkan\n");
  }
  
  void updateAndRender(float dt) {
    cameraAngle += input::getViewAngleInput();
    
    vec2 lateralMovement = input::getMovementVector() * (dt * 2);
    
    lateralMovement = rotate(lateralMovement, -cameraAngle.x);
    
    cameraPosition.x += lateralMovement.x;
    cameraPosition.z -= lateralMovement.y;
    
    PerFrameShaderData shaderData;
    shaderData.matrix = identity<mat4>();
    
    VkExtent2D extent = foundation->getSurfaceExtent();
    float aspect = extent.width / (float)extent.height;
    shaderData.matrix = perspective(radians(50.0f), aspect, 0.1f, 10.0f);
    shaderData.matrix = scale(shaderData.matrix, vec3(1, -1, 1));
    
    shaderData.matrix = rotate(shaderData.matrix, cameraAngle.y, vec3(1.0f, 0.0f, 0.0f));
    shaderData.matrix = rotate(shaderData.matrix, cameraAngle.x, vec3(0.0f, 1.0f, 0.0f));
    
    shaderData.matrix = translate(shaderData.matrix, -cameraPosition);
    
    pipeline->submit(pyramid);
    pipeline->submit(ground);
    pipeline->present(&shaderData);
  }

  void destroy() {
    delete pyramid;
    delete ground;
    delete pipeline;
    delete foundation;
  }
}



