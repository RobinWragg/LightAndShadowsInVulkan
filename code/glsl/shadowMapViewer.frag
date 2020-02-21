#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D shadowMap;

void main() {
  // The color is shrunk by a factor of 0.1 in order to fit the distances contained in the shadowMap (which are usually >1) into unit range [0,1]. If we didn't do this, everything would appear white.
  vec3 color3 = texture(shadowMap, texCoord).xyz * 0.07;
  outColor = vec4(color3, 1);
}