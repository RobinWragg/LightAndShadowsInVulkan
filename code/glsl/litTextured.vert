#version 450

layout(location = 0) in vec3 vertPosInMesh;
layout(location = 1) in vec3 vertNormalInMesh;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 vertPosInView;
layout(location = 1) out vec3 vertNormalInView;
layout(location = 2) out vec3 lightPosInView;
layout(location = 3) out vec3 vertPosInLightView;
layout(location = 4) out vec2 outTexCoord;
layout(location = 5) out mat3 normalMatrix;

layout(set = 0, binding = 0) uniform DrawCall {
  mat4 worldMatrix;
} drawCall;

layout(set = 1, binding = 0) uniform LightMatrices {
  mat4 view;
  mat4 proj;
} lightMatrices;

layout(set = 2, binding = 0) uniform Matrices {
  mat4 view;
  mat4 proj;
} matrices;

void main() {
  vec4 vertPosInWorld4 = drawCall.worldMatrix * vec4(vertPosInMesh, 1.0);
  vec4 vertPosInView4 = matrices.view * vertPosInWorld4;
  vertPosInView = vertPosInView4.xyz;
  
  gl_Position = matrices.proj * vertPosInView4;
  
  vertPosInLightView = (lightMatrices.view * vertPosInWorld4).xyz;
  
  // Transform the normal to view space
  normalMatrix = mat3(matrices.view * drawCall.worldMatrix);
  normalMatrix = transpose(inverse(normalMatrix));
  vertNormalInView = normalMatrix * vertNormalInMesh;
  
  // The light is of course at the origin in light-view space, so we can get its world position this way:
  vec4 lightPosInWorld4 = inverse(lightMatrices.view) * vec4(0, 0, 0, /* <- origin */ 1);
  lightPosInView = (matrices.view * lightPosInWorld4).xyz;
  
  outTexCoord = inTexCoord;
}




