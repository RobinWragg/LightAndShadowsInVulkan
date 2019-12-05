#include "main.h"

#include "GraphicsFoundation.h"
#include "GraphicsPipeline.h"
#include "DrawCall.h"
#include "input.h"

namespace graphics {
  GraphicsFoundation *foundation = nullptr;
  GraphicsPipeline *pipeline = nullptr;
  DrawCall *drawCall = nullptr;
  vec3 cameraPosition = {};

  void init(SDL_Window *window) {
    
    foundation = new GraphicsFoundation(window);
    
    pipeline = new GraphicsPipeline(foundation, true);
    
    vector<vec3> vertices = {
      {0, 0, 0}, {1, 0, 0}, {0, 1, 0},
      {0, 0, 0}, {0, 0, 1}, {1, 0, 0},
      {0, 0, 0}, {0, 1, 0}, {0, 0, 1},
      {1, 0, 0}, {0, 0, 1}, {0, 1, 0},
    };
    
    drawCall = new DrawCall(pipeline, vertices);
    
    printf("\nInitialised Vulkan\n");
  }
  
  void updateAndRender(float dt) {
    vec2 lateralMovement = input::getKeyVector() * dt;
    
    cameraPosition.x -= lateralMovement.x;
    cameraPosition.z -= lateralMovement.y;
    
    PerFrameShaderData shaderData;
    shaderData.matrix = identity<mat4>();
    shaderData.matrix = translate(shaderData.matrix, cameraPosition);
    shaderData.matrix = translate(shaderData.matrix, vec3(0, 0, 0.5));
    shaderData.matrix = scale(shaderData.matrix, vec3(1, 1, 0.5));
    shaderData.matrix = rotate(shaderData.matrix, (float)getTime(), vec3(1.0f, 0.0f, 0.0f));
    shaderData.matrix = rotate(shaderData.matrix, (float)getTime()*0.9f, vec3(0.0f, 1.0f, 0.0f));
    
    pipeline->submit(drawCall);
    pipeline->present(&shaderData);
  }

  void destroy() {
    delete drawCall;
    delete pipeline;
    delete foundation;
  }
}



