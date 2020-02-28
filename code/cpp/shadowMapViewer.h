#pragma once
#include "graphics.h"
#include "ShadowMap.h"

namespace shadowMapViewer {
  void init(vector<ShadowMap> *shadowMaps);
  void render(const gfx::SwapchainFrame *frame);
}