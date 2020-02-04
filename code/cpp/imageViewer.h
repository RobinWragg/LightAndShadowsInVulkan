#pragma once
#include <vulkan/vulkan.h>

namespace imageViewer {
  void init();
  void addToCommandBuffer(VkCommandBuffer cmdBuffer);
}