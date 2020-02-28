#version 450

layout(location = 0) in vec3 vertPosInMesh;
layout(location = 1) in vec3 vertNormalInMesh;

layout(location = 0) out vec3 vertPosInWorld;
layout(location = 1) out vec3 vertNormalInWorld;
layout(location = 2) out vec3 lightPosInWorld;

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
  vertPosInWorld = vertPosInWorld4.xyz;
  
  gl_Position = matrices.proj * matrices.view * vertPosInWorld4;
  
  // Transform the normal to world space
  mat3 normalMatrix = mat3(drawCall.worldMatrix);
  normalMatrix = transpose(inverse(normalMatrix));
  vertNormalInWorld = normalMatrix * vertNormalInMesh;
  
  lightPosInWorld = (inverse(lightMatrices.view) * vec4(0, 0, 0, 1)).xyz;
}




