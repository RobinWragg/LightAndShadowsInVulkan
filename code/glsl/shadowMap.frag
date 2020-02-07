#version 450

layout(location = 0) in vec4 viewPosition;
layout(location = 0) out vec4 outColor;

void main() {
  const float distance = length(viewPosition.xyz);
  const float debugLuminance = distance * 0.15;
  outColor = vec4(distance, debugLuminance, debugLuminance, 1);
}