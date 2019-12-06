#include "input.h"

namespace input {
  bool firstPersonMode = false;
  
  bool forward = false;
  bool backward = false;
  bool left = false;
  bool right = false;
  
  vec2 viewAngleInput = {};
  
  void handleKeyPress(SDL_Keycode code) {
    switch (code) {
      case SDLK_w: forward = true; break;
      case SDLK_s: backward = true; break;
      case SDLK_a: left = true; break;
      case SDLK_d: right = true; break;
    };
  }
  
  void handleKeyRelease(SDL_Keycode code) {
    switch (code) {
      case SDLK_w: forward = false; break;
      case SDLK_s: backward = false; break;
      case SDLK_a: left = false; break;
      case SDLK_d: right = false; break;
    };
  }
  
  void handleMouseClick(SDL_Window *window) {
    firstPersonMode = !firstPersonMode;
    
    if (firstPersonMode) {
      SDL_SetRelativeMouseMode(SDL_TRUE);
    } else {
      SDL_SetRelativeMouseMode(SDL_FALSE);
      
      int w, h;
      SDL_GetWindowSize(window, &w, &h);
      SDL_WarpMouseInWindow(window, w/2, h/2);
    }
  }
  
  void handleMouseMotion(int x, int y) {
    if (firstPersonMode) {
      float scale = 0.005f;
      viewAngleInput.x = x * scale;
      viewAngleInput.y = y * scale;
    } else {
      viewAngleInput.x = 0;
      viewAngleInput.y = 0;
    }
  }
  
  vec2 getViewAngleInput() {
    return viewAngleInput;
  }
  
  vec2 getMovementVector() {
    vec2 v = {};
    
    if (firstPersonMode) {
      if (forward) v.y = 1;
      else if (backward) v.y = -1;
      
      if (right) v.x = 1;
      else if (left) v.x = -1;
      
      if (length(v) > 0.0001) v = normalize(v);
    }
    
    return v;
  }
}







