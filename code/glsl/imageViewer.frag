#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D shadowMap;

void main() {
  outColor = texture(shadowMap, texCoord);
}