#pragma once

#define SHADOWMAP_RESOLUTION 4096
#define MAX_LIGHT_SUBSOURCE_COUNT 14
#define MAX_SHADOW_ANTI_ALIAS_SIZE 10
#define MSAA_SETTING VK_SAMPLE_COUNT_8_BIT

struct Settings {
  int subsourceCount = 1;
  
  enum {
    RING,
    SPIRAL
  } subsourceArrangement = SPIRAL;
  
  bool animateLightPos = false;
  bool renderTextures = false;
  bool renderNormalMaps = false;
  float ambReflection = 0.1;
  
  float sourceRadius = 0.1;
  int shadowAntiAliasSize = 0;
};

extern Settings settings;
