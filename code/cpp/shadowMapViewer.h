#pragma once
#include "graphics.h"
#include "ShadowMap.h"

namespace shadowMapViewer {
  void init(ShadowMap *shadowMap);
  void addToCommandBuffer(const gfx::SwapchainFrame *frame);
}