#version 450

layout(location = 0) in vec3 fragmentColor;
layout(location = 1) in vec4 worldPosition;
layout(location = 2) in vec3 lightPosition;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform LightViewMatrix {
  mat4 value;
} lightViewMatrix;

layout(set = 1, binding = 0) uniform LightProjMatrix {
  mat4 value;
} lightProjMatrix;

layout(set = 4, binding = 0) uniform sampler2D shadowMap;

void main() {
  vec4 vertLightProjectionPosition = lightProjMatrix.value * lightViewMatrix.value * worldPosition;
  vec2 texCoord = vec2(vertLightProjectionPosition.x * 0.5 + 0.5, vertLightProjectionPosition.y + 0.5);
  float vertToLightDistance = length(lightPosition - worldPosition.xyz);
  float texelColor = texture(shadowMap, texCoord).r;
  
  outColor = vec4(texelColor * 0.15, texelColor * 0.15, vertToLightDistance * 0.2, 1.0);
  // outColor.b = vertToLightDistance * 0.1;
  
  // texCoord debug
  if (abs(texCoord.x) <= 0.01 || (abs(texCoord.x) >= 0.99 && abs(texCoord.x) <= 1.01) || abs(texCoord.y) <= 0.01 || (abs(texCoord.y) >= 0.99 && abs(texCoord.y) <= 1.01)) {
    outColor = vec4(1, 1, 0, 1.0);
  }
  
}






