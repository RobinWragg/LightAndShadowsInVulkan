#pragma once
#include "graphics.h"
#include "ShadowMap.h"

namespace shadows {
  void init(vector<ShadowMap> *shadowMaps);
  void update();
  VkDescriptorSetLayout getMatricesDescSetLayout();
  VkDescriptorSet getMatricesDescSet();
  vector<vec2> getViewOffsets();
  void performRenderPasses(VkCommandBuffer cmdBuffer);
  vec3 getLightPos();
}