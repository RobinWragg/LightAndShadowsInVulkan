#pragma once

#define SHADOWMAP_RESOLUTION 2048
#define MAX_LIGHT_SUBSOURCE_COUNT 16
#define MAX_SHADOW_ANTI_ALIAS_SIZE 9

struct Settings {
  int subsourceCount = 1;
  
  enum {
    RING,
    SPIRAL
  } subsourceArrangement = SPIRAL;
  
  float sourceRadius = 0.1;
  int shadowAntiAliasSize = 0;
};

extern Settings settings;