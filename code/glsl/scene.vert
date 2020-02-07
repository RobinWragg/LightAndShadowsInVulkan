#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 fragmentColor;

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
  gl_Position = projMatrix.value * viewMatrix.value * pushConstant.model * vec4(position, 1.0);
  
  // Transform the normal to world space
  mat3 normalMatrix = mat3(pushConstant.model);
  normalMatrix = transpose(inverse(normalMatrix)); // TODO: Learn why this works. Inverting and transposing the matrix is required to handle scaled normals correctly, but I don't understand how it works at this time.
  vec3 worldNormal = normalize(normalMatrix * normal);
  
  fragmentColor = vec3(1, 1, 1) * dot(worldNormal, normalize(vec3(0.2, 0.4, 0.3)));
}




