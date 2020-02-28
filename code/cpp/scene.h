#pragma once
#include "graphics.h"
#include "ShadowMap.h"

namespace scene {
  void init(vector<ShadowMap> *shadowMaps);
  void update(float deltaTime);
  void performShadowMapRenderPasses(VkCommandBuffer cmdBuffer);
  void render(VkCommandBuffer cmdBuffer);
  VkImageView getShadowMapView();
}