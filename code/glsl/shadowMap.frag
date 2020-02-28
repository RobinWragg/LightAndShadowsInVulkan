#version 450

layout(location = 0) in vec4 vertPosInView;
layout(location = 0) out float outColor;

void main() {
  const float distance = length(vertPosInView.xyz);
  outColor = distance;
  // outColor = distance;
}