#pragma once
#include "graphics.h"
#include "ShadowMap.h"

namespace shadowMapViewer {
  void init(ShadowMap *shadowMap);
  void render(const gfx::SwapchainFrame *frame);
}