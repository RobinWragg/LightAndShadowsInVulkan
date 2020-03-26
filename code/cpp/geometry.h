#pragma once
#include "graphics.h"
#include "DrawCall.h"

namespace geometry {
  void recordCommands(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, bool texturedGeometry);
  void init();
  DrawCall * newSphereDrawCall(int resolution, bool smoothNormals);
}