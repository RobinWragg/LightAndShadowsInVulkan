#pragma once
#include "graphics.h"

namespace scene {
  void init();
  void addToCommandBuffer(VkCommandBuffer cmdBuffer, float dt);
}