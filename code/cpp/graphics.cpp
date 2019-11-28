#include "main.h"

#include "GraphicsFoundation.h"
#include "GraphicsPipeline.h"

namespace graphics {
  GraphicsFoundation *foundation = nullptr;
  GraphicsPipeline *pipeline = nullptr;

  void printAvailableDeviceLayers(VkPhysicalDevice device) {
    uint32_t layerCount;
    vkEnumerateDeviceLayerProperties(device, &layerCount, nullptr);
    vector<VkLayerProperties> deviceLayers(layerCount);
    vkEnumerateDeviceLayerProperties(device, &layerCount, deviceLayers.data());

    printf("\nAvailable device layers (deprecated API section):\n");
    for (const auto& layer : deviceLayers) printf("\t%s\n", layer.layerName);
  }

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

  void createVertexBuffers(uint32_t vertexCount, float vertices[],
    vector<VkBuffer> *vertexBuffers, vector<VkDeviceMemory> *vertexBufferMemSlots) {

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(float) * 3 * vertexCount;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    



    vertexBuffers->push_back(VkBuffer());
    auto result = vkCreateBuffer(foundation->device, &bufferInfo, nullptr, &vertexBuffers->back());
    SDL_assert(result == VK_SUCCESS);

    VkMemoryRequirements memoryReqs;
    vkGetBufferMemoryRequirements(foundation->device, vertexBuffers->back(), &memoryReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryReqs.size;
    allocInfo.memoryTypeIndex = pipeline->findMemoryType(
      memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vertexBufferMemSlots->push_back(VkDeviceMemory());
    result = vkAllocateMemory(foundation->device, &allocInfo, nullptr, &vertexBufferMemSlots->back());
    SDL_assert(result == VK_SUCCESS);
    result = vkBindBufferMemory(foundation->device, vertexBuffers->back(), vertexBufferMemSlots->back(), 0);
    SDL_assert(result == VK_SUCCESS);

    uint8_t * mappedMemory;
    result = vkMapMemory(foundation->device, vertexBufferMemSlots->back(), 0, bufferInfo.size, 0, (void**)&mappedMemory);
        SDL_assert(result == VK_SUCCESS);
        
    memcpy(mappedMemory, vertices, bufferInfo.size);
    vkUnmapMemory(foundation->device, vertexBufferMemSlots->back());
  }
  
  void createCommandBuffers(vector<VkBuffer> vertexBuffers, uint32_t vertexCount, vector<VkCommandBuffer> *commandBuffersOut) {

    commandBuffersOut->resize(pipeline->framebuffers.size());

    VkCommandBufferAllocateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferInfo.commandPool = pipeline->commandPool;
    bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // TODO: optimise by reusing commands (secondary buffer level)?
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
      beginInfo.flags = 0; // TODO: optimisation possible?
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
      vkCmdBindVertexBuffers((*commandBuffersOut)[i], 0, (uint32_t)vertexBuffers.size(), vertexBuffers.data(), offsets.data());

      vkCmdDraw((*commandBuffersOut)[i], vertexCount, 1, 0, 0);

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
  }

  // Ephemeral render buffers
  vector<VkBuffer> vertexBuffers;
  vector<VkDeviceMemory> vertexBufferMemSlots;
  vector<VkCommandBuffer> commandBuffers;

  void freeRenderBuffers() {
    vkFreeCommandBuffers(foundation->device, pipeline->commandPool, (uint32_t)commandBuffers.size(), commandBuffers.data());
    commandBuffers.resize(0);

    for (auto &buffer : vertexBuffers) vkDestroyBuffer(foundation->device, buffer, nullptr);
    vertexBuffers.resize(0);

    for (auto &slot : vertexBufferMemSlots) vkFreeMemory(foundation->device, slot, nullptr);
    vertexBufferMemSlots.resize(0);
  }

  void render() {
    uint32_t vertexCount = 6;
    float vertices[] = {
      -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0, -0.5, 0.5,
      -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.6
    };


    if (!commandBuffers.empty()) {
      // The queues may not have finished their commands from the last frame yet,
      // so we wait for everything to be finished before rebuilding the buffers.
      vkQueueWaitIdle(foundation->graphicsQueue);
      vkQueueWaitIdle(foundation->surfaceQueue);
      freeRenderBuffers();
    }

    createVertexBuffers(vertexCount, vertices, &vertexBuffers, &vertexBufferMemSlots);
    createCommandBuffers(vertexBuffers, vertexCount, &commandBuffers);

    // Submit commands
    uint32_t swapchainImageIndex = INT32_MAX;

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
    
    freeRenderBuffers();

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



