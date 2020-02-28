#version 450

layout(location = 0) in vec4 vertPosInView;
layout(location = 0) out float outColor;

void main() {
  outColor = length(vertPosInView.xyz);
}
