#pragma once
#include "graphics.h"
#include "ShadowMap.h"

namespace scene {
  void init(ShadowMap *shadowMap);
  void update(float deltaTime);
  void performShadowMapRenderPass(VkCommandBuffer cmdBuffer);
  void render(VkCommandBuffer cmdBuffer);
  VkImageView getShadowMapView();
}