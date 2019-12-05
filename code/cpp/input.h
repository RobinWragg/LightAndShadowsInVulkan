#pragma once

#include "main.h"

namespace input {
  void handleKeyPress(SDL_Keycode code);
  void handleKeyRelease(SDL_Keycode code);
  vec2 getKeyVector();
}