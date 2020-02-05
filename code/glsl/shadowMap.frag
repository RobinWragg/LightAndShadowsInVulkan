#version 450

layout(location = 0) in vec4 worldPosition;
layout(location = 0) out vec4 outColor;

void main() {
  float distance = length(worldPosition.xyz - vec3(2, 1, 0));
  outColor = vec4(distance, distance, distance, 1);
}