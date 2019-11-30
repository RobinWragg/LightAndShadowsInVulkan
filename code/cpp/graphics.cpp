#include "main.h"

#include "GraphicsFoundation.h"
#include "GraphicsPipeline.h"
#include "DrawCall.h"

namespace graphics {
  GraphicsFoundation *foundation = nullptr;
  GraphicsPipeline *pipeline = nullptr;
  DrawCall *drawCall = nullptr;

  VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT msgType,
    const VkDebugUtilsMessengerCallbackDataEXT *data,
    void *pUserData) {

    printf("\n");

    switch (severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: printf("verbose, "); break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: printf("info, "); break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: printf("WARNING, "); break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: printf("ERROR, "); break;
    default: printf("unknown, "); break;
    };

    switch (msgType) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: printf("general: "); break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: printf("validation: "); break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: printf("performance: "); break;
    default: printf("unknown: "); break;
    };

    printf("%s (%i objects reported)\n", data->pMessage, data->objectCount);
    fflush(stdout);

    switch (severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      SDL_assert_release(false);
      break;
    default: break;
    };

    return VK_FALSE;
  }

  void init(SDL_Window *window) {
    
    foundation = new GraphicsFoundation(window, debugCallback);
    
    pipeline = new GraphicsPipeline(foundation, true);
    
    vector<vec3> vertices = {
      {-0.5, 0.5, 0.1}, {0.5, 0.5, 0.1}, {0, -0.5, 0.1},
      {0, 0.5, 0.6}, {1, 0.5, 0.6}, {0.5, -0.5, 0.6},
    };
    
    drawCall = new DrawCall(pipeline, vertices);
    
    printf("\nInitialised Vulkan\n");
  }
  
  void render() {
    // Submit commands
    uint32_t swapchainImageIndex = INT32_MAX;
    
    fflush(stdout);
    auto result = vkAcquireNextImageKHR(foundation->device, pipeline->swapchain, UINT64_MAX /* no timeout */, pipeline->imageAvailableSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);
    SDL_assert(result == VK_SUCCESS);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &drawCall->commandBuffers[swapchainImageIndex];

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &pipeline->imageAvailableSemaphore;
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &pipeline->renderCompletedSemaphore;
    
    result = vkQueueSubmit(foundation->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    SDL_assert(result == VK_SUCCESS);

    // Present
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &pipeline->renderCompletedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &pipeline->swapchain;
    presentInfo.pImageIndices = &swapchainImageIndex;
    
    result = vkQueuePresentKHR(foundation->surfaceQueue, &presentInfo);
    SDL_assert(result == VK_SUCCESS);
  }

  void destroy() {
    vkQueueWaitIdle(foundation->graphicsQueue);
    vkQueueWaitIdle(foundation->surfaceQueue);
    
    vkFreeCommandBuffers(foundation->device, pipeline->commandPool, (uint32_t)drawCall->commandBuffers.size(), drawCall->commandBuffers.data());
    drawCall->commandBuffers.resize(0);

    vkDestroyBuffer(foundation->device, drawCall->vertexBuffer, nullptr);

    vkFreeMemory(foundation->device, drawCall->vertexBufferMemory, nullptr);
    
    vkDestroyCommandPool(foundation->device, pipeline->commandPool, nullptr);

    vkDeviceWaitIdle(foundation->device);

    vkDestroySemaphore(foundation->device, pipeline->imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(foundation->device, pipeline->renderCompletedSemaphore, nullptr);

    for (auto &buffer : pipeline->framebuffers) vkDestroyFramebuffer(foundation->device, buffer, nullptr);
    
    vkDestroyPipeline(foundation->device, pipeline->vkPipeline, nullptr);
    vkDestroyPipelineLayout(foundation->device, pipeline->pipelineLayout, nullptr);
    vkDestroyRenderPass(foundation->device, pipeline->renderPass, nullptr);

    for (auto view : pipeline->swapchainViews) vkDestroyImageView(foundation->device, view, nullptr);
    pipeline->swapchainViews.resize(0);

    pipeline->swapchainImages.resize(0);
    vkDestroySwapchainKHR(foundation->device, pipeline->swapchain, nullptr);
    
    delete foundation;
  }
}



