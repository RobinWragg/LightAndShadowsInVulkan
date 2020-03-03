#pragma once

struct Settings {
  int subsourceCount = 1;
  
  enum {
    RING,
    SPIRAL
  } subsourceArrangement = SPIRAL;
  
  float sourceRadius = 0.1;
};

extern Settings settings;