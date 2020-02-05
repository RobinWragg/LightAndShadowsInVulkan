#version 450

layout(location = 0) in vec4 worldPosition;
layout(location = 0) out vec4 outColor;

void main() {
  outColor = vec4(worldPosition.xyz, 1.0);
}