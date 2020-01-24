#include "GraphicsPipeline.h"
#include "DrawCall.h"

GraphicsPipeline::GraphicsPipeline() {
  
  createRenderPass();
  
  createDescriptorSetLayout(&perFrameDescriptorLayout);
  createDescriptorSetLayout(&drawCallDescriptorLayout);
  
  createVkPipeline();
  
  createCommandPool();
  
  depthImage = VK_NULL_HANDLE;
  depthImageMemory = VK_NULL_HANDLE;
  depthImageView = VK_NULL_HANDLE;
  setupDepthTesting();
  
  createFramebuffers();
  
  createSemaphores();
  
  createDescriptorPool();
  
  for (int i = 0; i < swapchainSize; i++) {
    createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(PerFrameUniform), &perFrameDescriptorBuffers[i], &perFrameDescriptorBuffersMemory[i]);
    createDescriptorSet(perFrameDescriptorLayout, perFrameDescriptorBuffers[i], &perFrameDescriptorSets[i]);
  }
  
  createCommandBuffers();
  
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

void GraphicsPipeline::createCommandBuffers() {
  VkCommandBufferAllocateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  bufferInfo.commandPool = commandPool;
  bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  bufferInfo.commandBufferCount = swapchainSize;
  auto result = vkAllocateCommandBuffers(device, &bufferInfo, commandBuffers);
  SDL_assert(result == VK_SUCCESS);
}

void GraphicsPipeline::fillCommandBuffer(uint32_t swapchainIndex) {
  
  VkCommandBuffer &cmdBuffer = commandBuffers[swapchainIndex];
  
  // Color clear value
  
  vector<VkClearValue> clearValues(2);

  clearValues[0].color.float32[0] = 0.5;
  clearValues[0].color.float32[1] = 0.7;
  clearValues[0].color.float32[2] = 1;
  clearValues[0].color.float32[3] = 1;
  
  clearValues[1].depthStencil.depth = 1;
  clearValues[1].depthStencil.stencil = 0;
  
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  beginInfo.pInheritanceInfo = nullptr;
  
  // This call will implicitly reset the command buffer if it has previously been filled
  auto result = vkBeginCommandBuffer(cmdBuffer, &beginInfo);
  SDL_assert(result == VK_SUCCESS);

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = framebuffers[swapchainIndex];
  
  renderPassInfo.clearValueCount = (int)clearValues.size();
  renderPassInfo.pClearValues = clearValues.data();

  renderPassInfo.renderArea.offset = { 0, 0 };
  renderPassInfo.renderArea.extent = getSurfaceExtent();
  
  vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);

  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0 /*per-frame set index*/, 1, &perFrameDescriptorSets[swapchainIndex], 0, nullptr);
  
  for (auto &sub : submissions) {
    // Set per-drawcall shader data
    setMemory(sub.drawCall->descriptorBuffersMemory[swapchainIndex], sizeof(DrawCallUniform), &sub.uniform);
    
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1 /*drawcall set index*/, 1, &sub.drawCall->descriptorSets[swapchainIndex], 0, nullptr);
    
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

  result = vkEndCommandBuffer(commandBuffers[swapchainIndex]);
  SDL_assert(result == VK_SUCCESS);
}

void GraphicsPipeline::setupDepthTesting() {
  SDL_assert_release(commandPool != VK_NULL_HANDLE);
  
  VkFormat format = VK_FORMAT_D32_SFLOAT;
  createImage(format, &depthImage, &depthImageMemory);

  depthImageView = createImageView(depthImage, format, VK_IMAGE_ASPECT_DEPTH_BIT);

  VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
  cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdBufferAllocInfo.commandPool = commandPool;
  cmdBufferAllocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  vkAllocateCommandBuffers(device, &cmdBufferAllocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  
  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = depthImage;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void GraphicsPipeline::createSemaphores() {
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  SDL_assert_release(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) == VK_SUCCESS);
  SDL_assert_release(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderCompletedSemaphore) == VK_SUCCESS);
}

vector<uint8_t> GraphicsPipeline::loadBinaryFile(const char *filename) {
  ifstream file(filename, ios::ate | ios::binary);

  printf("LOADING: %s\n", filename);
  SDL_assert_release(file.is_open());

  vector<uint8_t> bytes(file.tellg());
  file.seekg(0);
  file.read((char*)bytes.data(), bytes.size());
  file.close();

  return bytes;
}

VkPipelineShaderStageCreateInfo GraphicsPipeline::createShaderStage(const char *spirVFilePath, VkShaderStageFlagBits stage) {
  auto spirV = loadBinaryFile(spirVFilePath);

  VkShaderModuleCreateInfo moduleInfo = {};
  moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleInfo.codeSize = spirV.size();
  moduleInfo.pCode = (uint32_t*)spirV.data();

  VkShaderModule module = VK_NULL_HANDLE;
  SDL_assert_release(vkCreateShaderModule(device, &moduleInfo, nullptr, &module) == VK_SUCCESS);

  VkPipelineShaderStageCreateInfo stageInfo = {};
  stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

  stageInfo.stage = stage;

  stageInfo.module = module;
  stageInfo.pName = "main";

  return stageInfo;
}

void GraphicsPipeline::createCommandPool() {
  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndex;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.pNext = nullptr;

  SDL_assert_release(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) == VK_SUCCESS);
}

void GraphicsPipeline::createVkPipeline() {
  
  // TODO: refactor this function?
  
  vector<VkVertexInputBindingDescription> vertBindingDescs;
  
  VkVertexInputBindingDescription positionBindingDesc = {};
  positionBindingDesc.binding = 0;
  positionBindingDesc.stride = sizeof(vec3);
  positionBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  vertBindingDescs.push_back(positionBindingDesc);
  
  VkVertexInputBindingDescription normalBindingDesc = {};
  normalBindingDesc.binding = 1;
  normalBindingDesc.stride = sizeof(vec3);
  normalBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  vertBindingDescs.push_back(normalBindingDesc);
  
  vector<VkVertexInputAttributeDescription> vertAttribDescs;
  
  VkVertexInputAttributeDescription positionAttribDesc = {};
  positionAttribDesc.binding = 0;
  positionAttribDesc.location = 0;
  positionAttribDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
  positionAttribDesc.offset = 0;
  vertAttribDescs.push_back(positionAttribDesc);
  
  VkVertexInputAttributeDescription normalAttribDesc = {};
  normalAttribDesc.binding = 1;
  normalAttribDesc.location = 1;
  normalAttribDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
  normalAttribDesc.offset = 0;
  vertAttribDescs.push_back(normalAttribDesc);
  
  vector<VkPipelineShaderStageCreateInfo> shaderStages = {
    createShaderStage("basic.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
    createShaderStage("basic.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
  };

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertBindingDescs.size();
  vertexInputInfo.pVertexBindingDescriptions = vertBindingDescs.data();

  vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertAttribDescs.size();
  vertexInputInfo.pVertexAttributeDescriptions = vertAttribDescs.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = 0;
  
  auto extent = getSurfaceExtent();
  viewport.width = (float)extent.width;
  viewport.height = (float)extent.height;
  
  viewport.minDepth = 0;
  viewport.maxDepth = 1;

  VkRect2D scissor = {};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent = extent;

  VkPipelineViewportStateCreateInfo viewportInfo = {};
  viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportInfo.viewportCount = 1;
  viewportInfo.pViewports = &viewport;
  viewportInfo.scissorCount = 1;
  viewportInfo.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterInfo = {};
  rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterInfo.depthClampEnable = VK_FALSE;
  rasterInfo.rasterizerDiscardEnable = VK_FALSE;
  rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterInfo.lineWidth = 1;
  rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterInfo.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisamplingInfo = {};
  multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisamplingInfo.sampleShadingEnable = VK_FALSE;
  multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;

  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkPipelineLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  
  const int descriptorSetLayoutCount = 2;
  VkDescriptorSetLayout descriptorSetLayouts[descriptorSetLayoutCount] = {
    perFrameDescriptorLayout,
    drawCallDescriptorLayout
  };
  layoutInfo.setLayoutCount = descriptorSetLayoutCount;
  layoutInfo.pSetLayouts = descriptorSetLayouts;
  
  auto result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout);
  SDL_assert_release(result == VK_SUCCESS);

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

  pipelineInfo.stageCount = (int)shaderStages.size();
  pipelineInfo.pStages = shaderStages.data();

  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportInfo;
  
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};
  depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilInfo.depthTestEnable = VK_TRUE;
  depthStencilInfo.depthWriteEnable = VK_TRUE;
  depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS; // Lower depth values mean closer to 'camera'
  depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
  depthStencilInfo.stencilTestEnable = VK_FALSE;
  pipelineInfo.pDepthStencilState = &depthStencilInfo;

  pipelineInfo.pRasterizationState = &rasterInfo;
  pipelineInfo.pMultisampleState = &multisamplingInfo;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  SDL_assert_release(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipeline) == VK_SUCCESS);

  for (auto &stage : shaderStages) vkDestroyShaderModule(device, stage.module, nullptr);
}

VkAttachmentDescription GraphicsPipeline::createAttachmentDescription(VkFormat format, VkAttachmentStoreOp storeOp, VkImageLayout finalLayout) {
  VkAttachmentDescription description = {};

  description.format = format;
  description.samples = VK_SAMPLE_COUNT_1_BIT;
  description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  description.storeOp = storeOp;
  description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  description.finalLayout = finalLayout;
  
  return description;
}

void GraphicsPipeline::createRenderPass() {

  vector<VkAttachmentDescription> attachments = {};

  VkAttachmentDescription colorAttachment = createAttachmentDescription(surfaceFormat, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  attachments.push_back(colorAttachment);

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkAttachmentDescription depthAttachment = createAttachmentDescription(VK_FORMAT_D32_SFLOAT, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  attachments.push_back(depthAttachment);

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;
  
  
  
  

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  
  renderPassInfo.attachmentCount = (uint32_t)attachments.size();
  renderPassInfo.pAttachments = attachments.data();
  
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  
  VkSubpassDependency dep = createSubpassDependency();
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dep;

  SDL_assert_release(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) == VK_SUCCESS);
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
  setMemory(uniformMemory, sizeof(PerFrameUniform), perFrameUniform);
  
  // Fill the command buffer
  fillCommandBuffer(swapchainIndex);
  
  // Submit the command buffer
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[swapchainIndex];

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
  
  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  submitInfo.pWaitDstStageMask = &waitStage;

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &renderCompletedSemaphore;
  
  result = vkQueueSubmit(queue, 1, &submitInfo, fences[swapchainIndex]);
  SDL_assert(result == VK_SUCCESS);
  
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







