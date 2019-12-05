#pragma once

#include <cstdio>
#include <vector>
#include <set>
#include <thread>
#include <mutex>
#include <fstream>
#include <random>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <SDL2/SDL.h>

using namespace glm;
using namespace std;

double getTime();

namespace graphics {
  void init(SDL_Window *window);
  void updateAndRender(float dt);
  void destroy();
}
