#include "GraphicsPipeline.h"

class DrawCall {
public:
  DrawCall(const GraphicsPipeline *pipeline, const vector<vec3> &vertices);

private:
  const GraphicsPipeline *pipeline;
};