#version 450

layout(location = 0) in vec4 viewPosition;
layout(location = 0) out vec4 outColor;

void main() {
  const float distance = length(viewPosition.xyz);
  outColor = vec4(distance, distance, distance, 1);
}