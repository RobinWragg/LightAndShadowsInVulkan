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

  void init(SDL_Window *window) {
    foundation = new GraphicsFoundation(window);
    pipeline = new GraphicsPipeline(foundation, true);
    
    cameraPosition.x = 0;
    cameraPosition.y = 1;
    cameraPosition.z = 3;
    
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
    vec2 lateralMovement = input::getKeyVector() * (dt * 2);
    
    cameraPosition.x += lateralMovement.x;
    cameraPosition.z -= lateralMovement.y;
    
    PerFrameShaderData shaderData;
    shaderData.matrix = identity<mat4>();
    
    VkExtent2D extent = foundation->getSurfaceExtent();
    float aspect = extent.width / (float)extent.height;
    shaderData.matrix = perspective(radians(50.0f), aspect, 0.1f, 10.0f);
    shaderData.matrix = scale(shaderData.matrix, vec3(1, -1, 1));
    
    // TODO: viewing angle rotation goes here
    
    shaderData.matrix = translate(shaderData.matrix, -cameraPosition);
    shaderData.matrix = rotate(shaderData.matrix, (float)getTime()*0.5f, vec3(0.0f, 1.0f, 0.0f));
    
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



