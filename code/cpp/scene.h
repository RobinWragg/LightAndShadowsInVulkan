#pragma once
#include "graphics.h"

namespace scene {
  void init(SDL_Window *window);
  void updateAndRender(float dt, const gfx::SwapchainFrame *frame, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore);
}