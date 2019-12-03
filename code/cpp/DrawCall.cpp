#include "DrawCall.h"

DrawCall::DrawCall(const GraphicsPipeline *pipeline, const vector<vec3> &vertices) {
  this->pipeline = pipeline;
  device = pipeline->foundation->device; // Shorthand for accessing the device
  
  createVertexBuffer(vertices);
  
  createCommandBuffers(vertices.size());
}

DrawCall::~DrawCall() {
  vkFreeCommandBuffers(device, pipeline->commandPool, GraphicsPipeline::swapchainSize, commandBuffers);
  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);
}

void DrawCall::createVertexBuffer(const vector<vec3> &vertices) {
  
  uint64_t dataSize = sizeof(vertices[0]) * vertices.size();
  uint8_t *data = (uint8_t*)vertices.data();
  pipeline->foundation->createVkBuffer(dataSize, data, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vertexBuffer, &vertexBufferMemory);
}

void DrawCall::createCommandBuffers(uint64_t vertexCount) {

  VkCommandBufferAllocateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  bufferInfo.commandPool = pipeline->commandPool;
  bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  bufferInfo.commandBufferCount = GraphicsPipeline::swapchainSize;
  auto result = vkAllocateCommandBuffers(device, &bufferInfo, commandBuffers);
  SDL_assert(result == VK_SUCCESS);

  vector<VkClearValue> clearValues;

  // Color clear value
  clearValues.push_back(VkClearValue());
  clearValues.back().color = { 0, 1, 0, 1 };
  clearValues.back().depthStencil = {};

  if (pipeline->depthTestingEnabled) {
    // Depth/stencil clear value
    clearValues.push_back(VkClearValue());
    clearValues.back().color = {};
    clearValues.back().depthStencil = { 1, 0 };
  }

  for (int i = 0; i < GraphicsPipeline::swapchainSize; i++) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    // Removes the need to wait on queues during submission/presentation
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    
    beginInfo.pInheritanceInfo = nullptr;
    result = vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
    SDL_assert(result == VK_SUCCESS);

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pipeline->renderPass;
    renderPassInfo.framebuffer = pipeline->framebuffers[i];

    renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = pipeline->foundation->getSurfaceExtent();

    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkPipeline);

    vector<VkDeviceSize> offsets = {0,0,0,0};
    vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &vertexBuffer, offsets.data());

    vkCmdDraw(commandBuffers[i], (uint32_t)vertexCount, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffers[i]);

    result = vkEndCommandBuffer(commandBuffers[i]);
    SDL_assert(result == VK_SUCCESS);
  }

  clearValues.resize(0);
}




