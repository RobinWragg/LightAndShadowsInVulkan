#pragma once
#include "graphics.h"
#include "DrawCall.h"

namespace geometry {
  void renderAllGeometryWithoutSamplers(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout);
  void renderBareGeometry(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout);
  void renderTexturedGeometry(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout);
  void renderTexturedNormalMappedGeometry(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout);
  void init();
  DrawCall * newSphereDrawCall(int resolution, bool smoothNormals);
}