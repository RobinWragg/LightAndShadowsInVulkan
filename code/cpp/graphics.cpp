#include "main.h"

#include "GraphicsFoundation.h"
#include "GraphicsPipeline.h"

namespace graphics {
  GraphicsFoundation *foundation = nullptr;
  GraphicsPipeline *pipeline = nullptr;
  
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  vector<VkCommandBuffer> commandBuffers;

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

  void createVertexBuffer(const vector<vec3> &vertices,
    VkBuffer *vertexBuffer, VkDeviceMemory *vertexBufferMemory) {

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(vertices[0]) * vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    



    auto result = vkCreateBuffer(foundation->device, &bufferInfo, nullptr, vertexBuffer);
    SDL_assert(result == VK_SUCCESS);

    VkMemoryRequirements memoryReqs;
    vkGetBufferMemoryRequirements(foundation->device, *vertexBuffer, &memoryReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryReqs.size;
    allocInfo.memoryTypeIndex = foundation->findMemoryType(
      memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    result = vkAllocateMemory(foundation->device, &allocInfo, nullptr, vertexBufferMemory);
    SDL_assert(result == VK_SUCCESS);
    result = vkBindBufferMemory(foundation->device, *vertexBuffer, *vertexBufferMemory, 0);
    SDL_assert(result == VK_SUCCESS);

    uint8_t * mappedMemory;
    result = vkMapMemory(foundation->device, *vertexBufferMemory, 0, bufferInfo.size, 0, (void**)&mappedMemory);
        SDL_assert(result == VK_SUCCESS);
        
    memcpy(mappedMemory, vertices.data(), bufferInfo.size);
    vkUnmapMemory(foundation->device, *vertexBufferMemory);
  }
  
  void createCommandBuffers(VkBuffer vertexBuffer, uint64_t vertexCount, vector<VkCommandBuffer> *commandBuffersOut) {

    commandBuffersOut->resize(pipeline->framebuffers.size());

    VkCommandBufferAllocateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferInfo.commandPool = pipeline->commandPool;
    bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferInfo.commandBufferCount = (int)commandBuffersOut->size();
    auto result = vkAllocateCommandBuffers(foundation->device, &bufferInfo, commandBuffersOut->data());
    SDL_assert(result == VK_SUCCESS);

    vector<VkClearValue> clearValues;

    // Color clear value
    clearValues.push_back(VkClearValue());
    clearValues.back().color = { 0, 1, 0, 1 };
    clearValues.back().depthStencil = {};

    if (pipeline->enableDepthTesting) {
      // Depth/stencil clear value
      clearValues.push_back(VkClearValue());
      clearValues.back().color = {};
      clearValues.back().depthStencil = { 1, 0 };
    }

    for (int i = 0; i < commandBuffersOut->size(); i++) {
      VkCommandBufferBeginInfo beginInfo = {};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = 0;
      beginInfo.pInheritanceInfo = nullptr;
      result = vkBeginCommandBuffer((*commandBuffersOut)[i], &beginInfo);
      SDL_assert(result == VK_SUCCESS);

      VkRenderPassBeginInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = pipeline->renderPass;
      renderPassInfo.framebuffer = pipeline->framebuffers[i];

      renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
      renderPassInfo.pClearValues = clearValues.data();

      renderPassInfo.renderArea.offset = { 0, 0 };
      renderPassInfo.renderArea.extent = pipeline->getSurfaceExtent();

      vkCmdBeginRenderPass((*commandBuffersOut)[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline((*commandBuffersOut)[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkPipeline);

      vector<VkDeviceSize> offsets = {0,0,0,0};
      vkCmdBindVertexBuffers((*commandBuffersOut)[i], 0, 1, &vertexBuffer, offsets.data());

      vkCmdDraw((*commandBuffersOut)[i], (uint32_t)vertexCount, 1, 0, 0);

      vkCmdEndRenderPass((*commandBuffersOut)[i]);

      result = vkEndCommandBuffer((*commandBuffersOut)[i]);
      SDL_assert(result == VK_SUCCESS);
    }

    clearValues.resize(0);
  }

  void init(SDL_Window *window) {
    
    foundation = new GraphicsFoundation(window, debugCallback);
    
    printf("\nInitialised Vulkan\n");
    
    pipeline = new GraphicsPipeline(foundation, true);
    
    
    
    vector<vec3> vertices = {
      {-0.5, 0.5, 0.5}, {0.5, 0.5, 0.5}, {0, -0.5, 0.5},
      {-0.5, 0.5, 0.5}, {0.5, 0.5, 0.5}, {0.5, -0.5, 0.6}
    };

    createVertexBuffer(vertices, &vertexBuffer, &vertexBufferMemory);
    createCommandBuffers(vertexBuffer, vertices.size(), &commandBuffers);
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
    submitInfo.pCommandBuffers = &commandBuffers[swapchainImageIndex];

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &pipeline->imageAvailableSemaphore;
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &pipeline->renderCompletedSemaphore;
    
    vkQueueWaitIdle(foundation->graphicsQueue);
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

    vkQueueWaitIdle(foundation->surfaceQueue);
    result = vkQueuePresentKHR(foundation->surfaceQueue, &presentInfo);
    SDL_assert(result == VK_SUCCESS);
  }

  void destroy() {
    vkQueueWaitIdle(foundation->graphicsQueue);
    vkQueueWaitIdle(foundation->surfaceQueue);
    
    
    
    
    
    
    
    vkFreeCommandBuffers(foundation->device, pipeline->commandPool, (uint32_t)commandBuffers.size(), commandBuffers.data());
    commandBuffers.resize(0);

    vkDestroyBuffer(foundation->device, vertexBuffer, nullptr);

    vkFreeMemory(foundation->device, vertexBufferMemory, nullptr);
    
    
    

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



