#include "input.h"

namespace input {
  bool up = false;
  bool down = false;
  bool left = false;
  bool right = false;
  
  void handleKeyPress(SDL_Keycode code) {
    switch (code) {
      case SDLK_w: up = true; break;
      case SDLK_s: down = true; break;
      case SDLK_a: left = true; break;
      case SDLK_d: right = true; break;
    };
  }
  
  void handleKeyRelease(SDL_Keycode code) {
    switch (code) {
      case SDLK_w: up = false; break;
      case SDLK_s: down = false; break;
      case SDLK_a: left = false; break;
      case SDLK_d: right = false; break;
    };
  }
  
  vec2 getKeyVector() {
    vec2 v;
    
    if (up) v.y = 1;
    else if (down) v.y = -1;
    
    if (right) v.x = 1;
    else if (left) v.x = -1;
    
    if (length(v) > 0.0001) v = normalize(v);
    
    return v;
  }
}







