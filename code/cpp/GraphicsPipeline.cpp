#include "GraphicsPipeline.h"
#include "DrawCall.h"

GraphicsPipeline::GraphicsPipeline() {
  
  renderPass = createRenderPass();
  
  createDescriptorSetLayout(&perFrameDescriptorLayout);
  createDescriptorSetLayout(&drawCallDescriptorLayout);
  
  createVkPipeline();
  
  createFramebuffers();
  
  createSemaphores();
  
  createDescriptorPool();
  
  for (int i = 0; i < swapchainSize; i++) {
    createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(PerFrameUniform), &perFrameDescriptorBuffers[i], &perFrameDescriptorBuffersMemory[i]);
    createDescriptorSet(perFrameDescriptorLayout, perFrameDescriptorBuffers[i], &perFrameDescriptorSets[i]);
  }
  
  for (int i = 0; i < swapchainSize; i++) {
    commandBuffers[i] = createCommandBuffer();
  }
  
  createFences();
}

GraphicsPipeline::~GraphicsPipeline() {
  vkFreeCommandBuffers(device, commandPool, swapchainSize, commandBuffers);
  
  vkDestroyCommandPool(device, commandPool, nullptr);

  vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
  vkDestroySemaphore(device, renderCompletedSemaphore, nullptr);
  
  vkDestroyPipeline(device, vkPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);

  for (int i = 0; i < swapchainSize; i++) {
    vkDestroyFramebuffer(device, framebuffers[i], nullptr);
    vkDestroyFence(device, fences[i], nullptr);
    
    vkDestroyBuffer(device, perFrameDescriptorBuffers[i], nullptr);
    vkFreeMemory(device, perFrameDescriptorBuffersMemory[i], nullptr);
  }
  
  vkDestroyDescriptorSetLayout(device, perFrameDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, drawCallDescriptorLayout, nullptr);
  
  vkDestroyDescriptorPool(device, descriptorPool, nullptr); // Also destroys the descriptor sets
}

void GraphicsPipeline::createFences() {
  VkFenceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  
  for (int i = 0; i < swapchainSize; i++) {
    vkCreateFence(device, &createInfo, nullptr, &fences[i]);
  }
}

void GraphicsPipeline::createDescriptorPool() {
  VkDescriptorPoolSize poolSize = {};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = swapchainSize * maxDescriptors;
  
  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.maxSets = swapchainSize * maxDescriptors;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.flags = 0;
  
  auto result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
  SDL_assert_release(result == VK_SUCCESS);
}

void GraphicsPipeline::createDescriptorSet(VkDescriptorSetLayout layout, VkBuffer buffer, VkDescriptorSet *setOut) const {
  
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;
  
  auto result = vkAllocateDescriptorSets(device, &allocInfo, setOut);
  SDL_assert_release(result == VK_SUCCESS);
  
  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = buffer;
  bufferInfo.offset = 0;
  bufferInfo.range = VK_WHOLE_SIZE;
  
  VkWriteDescriptorSet writeDescriptorSet = {};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.dstSet = *setOut;
  writeDescriptorSet.dstBinding = 0;
  writeDescriptorSet.dstArrayElement = 0;
  writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.pBufferInfo = &bufferInfo;
  
  vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}

void GraphicsPipeline::createDescriptorSetLayout(VkDescriptorSetLayout *layoutOut) const {
  VkDescriptorSetLayoutBinding layoutBinding = {};
  layoutBinding.binding = 0;
  layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layoutBinding.descriptorCount = 1;
  layoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
  
  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &layoutBinding;
  
  auto result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, layoutOut);
  SDL_assert_release(result == VK_SUCCESS);
}

void GraphicsPipeline::fillCommandBuffer(uint32_t swapchainIndex, const PerFrameUniform *perFrameUniform) {
  
  VkCommandBuffer &cmdBuffer = commandBuffers[swapchainIndex];
  
  beginCommandBuffer(cmdBuffer);

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = framebuffers[swapchainIndex];
  
  vector<VkClearValue> clearValues(2);
  
  clearValues[0].color.float32[0] = 0.5;
  clearValues[0].color.float32[1] = 0.7;
  clearValues[0].color.float32[2] = 1;
  clearValues[0].color.float32[3] = 1;
  
  clearValues[1].depthStencil.depth = 1;
  clearValues[1].depthStencil.stencil = 0;
  
  renderPassInfo.clearValueCount = (int)clearValues.size();
  renderPassInfo.pClearValues = clearValues.data();

  renderPassInfo.renderArea.offset = { 0, 0 };
  renderPassInfo.renderArea.extent = getSurfaceExtent();
  
  vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);

  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0 /*per-frame set index*/, 1, &perFrameDescriptorSets[swapchainIndex], 0, nullptr);
  
  vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(*perFrameUniform), perFrameUniform);
  
  for (auto &sub : submissions) {
    vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(*perFrameUniform), sizeof(sub.uniform), &sub.uniform);
    
    // Set per-drawcall shader data
    // setBufferMemory(sub.drawCall->descriptorBuffersMemory[swapchainIndex], sizeof(DrawCallUniform), &sub.uniform);
    
    // vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1 /*drawcall set index*/, 1, &sub.drawCall->descriptorSets[swapchainIndex], 0, nullptr);
    
    const int bufferCount = 2;
    VkBuffer buffers[bufferCount] = {
      sub.drawCall->positionBuffer,
      sub.drawCall->normalBuffer
    };
    VkDeviceSize offsets[bufferCount] = {0, 0};
    vkCmdBindVertexBuffers(cmdBuffer, 0, bufferCount, buffers, offsets);
    
    vkCmdDraw(cmdBuffer, sub.drawCall->vertexCount, 1, 0, 0);
  }
  
  submissions.resize(0);
  
  vkCmdEndRenderPass(commandBuffers[swapchainIndex]);

  auto result = vkEndCommandBuffer(commandBuffers[swapchainIndex]);
  SDL_assert(result == VK_SUCCESS);
}

void GraphicsPipeline::createSemaphores() {
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  SDL_assert_release(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) == VK_SUCCESS);
  SDL_assert_release(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderCompletedSemaphore) == VK_SUCCESS);
}

void GraphicsPipeline::createVkPipeline() {
  const int descriptorSetLayoutCount = 2;
  VkDescriptorSetLayout descriptorSetLayouts[descriptorSetLayoutCount] = {
    perFrameDescriptorLayout,
    drawCallDescriptorLayout
  };
  pipelineLayout = createPipelineLayout(descriptorSetLayouts, descriptorSetLayoutCount);
  vkPipeline = createPipeline(pipelineLayout, renderPass);
}

void GraphicsPipeline::submit(DrawCall *drawCall, const DrawCallUniform *uniform) {
  Submission sub;
  sub.drawCall = drawCall;
  sub.uniform = *uniform;
  submissions.push_back(sub);
}

void GraphicsPipeline::present(const PerFrameUniform *perFrameUniform) {
  
  // Get next swapchain image
  uint32_t swapchainIndex = INT32_MAX;
  auto result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX /* no timeout */, imageAvailableSemaphore, VK_NULL_HANDLE, &swapchainIndex);
  SDL_assert(result == VK_SUCCESS);
  
  // Wait for the command buffer to finish executing
  vkWaitForFences(device, 1, &fences[swapchainIndex], VK_TRUE, INT64_MAX);
  vkResetFences(device, 1, &fences[swapchainIndex]);
  
  // Set per-frame shader data
  VkDeviceMemory &uniformMemory = perFrameDescriptorBuffersMemory[swapchainIndex];
  setBufferMemory(uniformMemory, sizeof(PerFrameUniform), perFrameUniform);
  
  // Fill the command buffer
  fillCommandBuffer(swapchainIndex, perFrameUniform);
  
  // Submit the command buffer
  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  submitCommandBuffer(commandBuffers[swapchainIndex], imageAvailableSemaphore, waitStage, renderCompletedSemaphore, fences[swapchainIndex]);
  
  // Present
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &renderCompletedSemaphore;

  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = &swapchainIndex;
  
  result = vkQueuePresentKHR(queue, &presentInfo);
  SDL_assert(result == VK_SUCCESS);
}

void GraphicsPipeline::createFramebuffers() {

  for (int i = 0; i < swapchainSize; i++) {
    vector<VkImageView> attachments = { swapchainViews[i], depthImageView };

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = (uint32_t)attachments.size();
    framebufferInfo.pAttachments = attachments.data();
    
    auto extent = getSurfaceExtent();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    
    framebufferInfo.layers = 1;

    SDL_assert_release(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) == VK_SUCCESS);
  }
}







