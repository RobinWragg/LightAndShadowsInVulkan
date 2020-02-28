#pragma once
#include "graphics.h"
#include "DrawCall.h"

namespace geometry {
  void addGeometryToCommandBuffer(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout);
  void init();
  DrawCall * newSphereDrawCall(int resolution, bool smoothNormals);
}