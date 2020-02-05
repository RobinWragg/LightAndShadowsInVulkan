#pragma once
#include "graphics.h"

namespace imageViewer {
  void init();
  void addToCommandBuffer(const gfx::SwapchainFrame *frame);
}