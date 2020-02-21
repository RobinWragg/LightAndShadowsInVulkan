#version 450

layout(location = 0) in vec3 vertPosInMesh;
layout(location = 1) in vec3 vertNormalInMesh;

layout(location = 0) out vec3 vertPosInWorld;
layout(location = 1) out vec3 vertNormalInWorld;
layout(location = 2) out vec3 lightPosInWorld;

layout(set = 0, binding = 0) uniform LightViewMatrix {
  mat4 value;
} lightViewMatrix;

layout(set = 2, binding = 0) uniform ViewMatrix {
  mat4 value;
} viewMatrix;

layout(set = 3, binding = 0) uniform ProjMatrix {
  mat4 value;
} projMatrix;

layout(push_constant) uniform PushConstant {
  mat4 model;
} pushConstant;

void main() {
  vec4 vertPosInWorld4 = pushConstant.model * vec4(vertPosInMesh, 1.0);
  vertPosInWorld = vertPosInWorld4.xyz;
  
  gl_Position = projMatrix.value * viewMatrix.value * vertPosInWorld4;
  
  // Transform the normal to world space
  mat3 normalMatrix = mat3(pushConstant.model);
  normalMatrix = transpose(inverse(normalMatrix)); // TODO: Learn why this works. Inverting and transposing the matrix is required to handle scaled normals correctly, but I don't understand how it works at this time.
  vertNormalInWorld = normalize(normalMatrix * vertNormalInMesh);
  
  lightPosInWorld = (inverse(lightViewMatrix.value) * vec4(0, 0, 0, 1)).xyz;
}




