#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 fragmentColor;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D shadowMap;

void main() {
  vec4 texColor = texture(shadowMap, texCoord) * 0.5;
  outColor = vec4(texColor.rgb * texColor.a, 1);
}