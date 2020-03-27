#version 450

layout(location = 0) in vec3 vertPosInMesh;
layout(location = 1) in vec3 vertNormalInMesh;
layout(location = 2) in vec2 vertTexCoordInMesh;

layout(location = 0) out vec3 vertPosInWorld;
layout(location = 2) out vec3 lightPosInWorld;
layout(location = 3) out vec2 fragTexCoord;

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
  
  lightPosInWorld = (inverse(lightMatrices.view) * vec4(0, 0, 0, 1)).xyz;
  
  fragTexCoord = vertTexCoordInMesh;
}




