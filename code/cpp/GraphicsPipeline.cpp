#include "GraphicsPipeline.h"
#include "DrawCall.h"

GraphicsPipeline::GraphicsPipeline() {
  
  pipelineLayout = createPipelineLayout(nullptr, 0);
  
  vkPipeline = createPipeline(pipelineLayout, renderPass);
  
  createSemaphores();
}

void GraphicsPipeline::beginCommandBuffer(const SwapchainFrame &frame, const mat4 &viewProjectionMatrix) {

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = frame.framebuffer;
  
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
  
  // Wait for the command buffer to finish executing
  vkWaitForFences(device, 1, &frame.cmdBufferFence, VK_TRUE, INT64_MAX);
  vkResetFences(device, 1, &frame.cmdBufferFence);
  
  gfx::beginCommandBuffer(frame.cmdBuffer);
  
  vkCmdBeginRenderPass(frame.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
  
  vkCmdPushConstants(frame.cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(viewProjectionMatrix), &viewProjectionMatrix);
}

void GraphicsPipeline::endCommandBuffer(const SwapchainFrame &frame) {
  vkCmdEndRenderPass(frame.cmdBuffer);

  auto result = vkEndCommandBuffer(frame.cmdBuffer);
  SDL_assert(result == VK_SUCCESS);
}

void GraphicsPipeline::createSemaphores() {
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  SDL_assert_release(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) == VK_SUCCESS);
  SDL_assert_release(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderCompletedSemaphore) == VK_SUCCESS);
}

void GraphicsPipeline::submit(const DrawCall *drawCall) {
  submissions.push_back(*drawCall);
}

// const SwapchainFrame& GraphicsPipeline::getNextFrame() {
  
// }

void GraphicsPipeline::present(const mat4 &viewProjectionMatrix) {
  
  // Get next swapchain image
  // Get the next swapchain image and signal the semaphore.
  uint32_t swapchainIndex = INT32_MAX;
  auto result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX /* no timeout */, imageAvailableSemaphore, VK_NULL_HANDLE, &swapchainIndex);
  SDL_assert(result == VK_SUCCESS);
  
  SwapchainFrame &frame = swapchainFrames[swapchainIndex];
  
  // Fill the command buffer
  beginCommandBuffer(frame, viewProjectionMatrix);
  
  for (auto &sub : submissions) {
    sub.addToCmdBuffer(frame.cmdBuffer, pipelineLayout);
  }
  submissions.resize(0);
  
  endCommandBuffer(frame);
  
  // Submit the command buffer
  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  submitCommandBuffer(frame.cmdBuffer, imageAvailableSemaphore, waitStage, renderCompletedSemaphore, frame.cmdBufferFence);
  
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







