#pragma once
#include "graphics.h"
#include "ShadowMap.h"

namespace shadows {
  void init(vector<ShadowMap> *shadowMaps);
  void update();
  VkDescriptorSetLayout getMatricesDescSetLayout();
  VkDescriptorSet getMatricesDescSet();
  void performRenderPasses(VkCommandBuffer cmdBuffer);
  vec3 getLightPos();
}