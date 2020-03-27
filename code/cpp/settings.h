#pragma once

#define SHADOWMAP_RESOLUTION 4096
#define MAX_LIGHT_SUBSOURCE_COUNT 14
#define MAX_SHADOW_ANTI_ALIAS_SIZE 9
#define MSAA_SETTING VK_SAMPLE_COUNT_4_BIT

struct Settings {
  int subsourceCount = 1;
  
  enum {
    RING,
    SPIRAL
  } subsourceArrangement = SPIRAL;
  
  bool animateLightPos = false;
  bool renderTextures = false;
  bool renderNormalMaps = false;
  
  float sourceRadius = 0.1;
  int shadowAntiAliasSize = 0;
};

extern Settings settings;
