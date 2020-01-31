#pragma once

// Disable warnings for conversions from double to float. Doubles aren't used in this project; disabling these warnings just means I don't have to put an "f" after every float literal.
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )

#include <cstdio>
#include <vector>
#include <set>
#include <thread>
#include <mutex>
#include <fstream>
#include <random>

#define M_TAU (2*M_PI)

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/rotate_vector.hpp>

#ifdef __APPLE__
#include <SDL2/SDL.h> // This is the include syntax for macOS frameworks
#include <SDL2/SDL_vulkan.h>
#else
#include <SDL.h>
#include <SDL_vulkan.h>
#endif

using namespace glm;
using namespace std;

vector<uint8_t> loadBinaryFile(const char *filename);
double getTime();

#include "scene.h"


