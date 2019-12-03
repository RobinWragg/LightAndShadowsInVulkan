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
    
    
    
    
    vkDestroyCommandPool(foundation->device, pipeline->commandPool, nullptr);

    vkDestroySemaphore(foundation->device, pipeline->imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(foundation->device, pipeline->renderCompletedSemaphore, nullptr);

    for (int i = 0; i < GraphicsPipeline::swapchainSize; i++) {
      vkDestroyFramebuffer(foundation->device, pipeline->framebuffers[i], nullptr);
    }
    
    vkDestroyPipeline(foundation->device, pipeline->vkPipeline, nullptr);
    vkDestroyPipelineLayout(foundation->device, pipeline->pipelineLayout, nullptr);
    vkDestroyRenderPass(foundation->device, pipeline->renderPass, nullptr);

    for (int i = 0; i < GraphicsPipeline::swapchainSize; i++) {
      vkDestroyImageView(foundation->device, pipeline->swapchainViews[i], nullptr);
    }
    
    vkDestroySwapchainKHR(foundation->device, pipeline->swapchain, nullptr);
    
    delete foundation;
  }
}



