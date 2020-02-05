#pragma once
#include "graphics.h"

namespace scene {
  void init();
  void update(float deltaTime);
  void performShadowMapRenderPass(VkCommandBuffer cmdBuffer);
  void renderScene(VkCommandBuffer cmdBuffer);
  VkImageView getShadowMapView();
}