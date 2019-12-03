#include "main.h"

#include "GraphicsFoundation.h"
#include "GraphicsPipeline.h"
#include "DrawCall.h"

namespace graphics {
  GraphicsFoundation *foundation = nullptr;
  GraphicsPipeline *pipeline = nullptr;
  DrawCall *drawCall = nullptr;

  void init(SDL_Window *window) {
    
    foundation = new GraphicsFoundation(window);
    
    pipeline = new GraphicsPipeline(foundation, true);
    
    vector<vec3> vertices = {
      {-0.5, 0.5, 0.1}, {0.5, 0.5, 0.1}, {0, -0.5, 0.1},
      {0, 0.5, 0.6}, {1, 0.5, 0.6}, {0.5, -0.5, 0.6},
    };
    
    drawCall = new DrawCall(pipeline, vertices);
    
    printf("\nInitialised Vulkan\n");
  }
  
  void render() {
    pipeline->submit(drawCall);
    pipeline->present();
  }

  void destroy() {
    
    delete drawCall;
    delete pipeline;
    delete foundation;
  }
}



