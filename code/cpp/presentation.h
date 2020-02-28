#pragma once
#include "graphics.h"
#include "ShadowMap.h"

namespace presentation {
  void init(VkDescriptorSetLayout shadowMapSamplerDescSetLayout);
  void update(float deltaTime);
  void render(VkCommandBuffer cmdBuffer, vector<ShadowMap> *shadowMaps);
}