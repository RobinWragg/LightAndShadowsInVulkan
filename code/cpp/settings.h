#pragma once

#define SHADOWMAP_RESOLUTION 2048
#define MAX_LIGHT_SUBSOURCE_COUNT 14
#define MAX_SHADOW_ANTI_ALIAS_SIZE 10
#define MSAA_SETTING VK_SAMPLE_COUNT_8_BIT

struct Settings {
  int subsourceCount = 8;
  
  enum {
    RING,
    SPIRAL
  } subsourceArrangement = SPIRAL;
  
  bool animateLightPos = true;
  bool renderTextures = true;
  bool renderNormalMaps = true;
  float ambReflection = 0.2;
  
  float sourceRadius = 0.4;
  int shadowAntiAliasSize = 2;
};

extern Settings settings;
