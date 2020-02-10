#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 fragmentColor;
layout(location = 1) out vec4 worldPosition;
layout(location = 2) out vec3 lightPosition;

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
  worldPosition = pushConstant.model * vec4(position, 1.0);
  
  gl_Position = projMatrix.value * viewMatrix.value * worldPosition;
  
  // Transform the normal to world space
  mat3 normalMatrix = mat3(pushConstant.model);
  normalMatrix = transpose(inverse(normalMatrix)); // TODO: Learn why this works. Inverting and transposing the matrix is required to handle scaled normals correctly, but I don't understand how it works at this time.
  vec3 worldNormal = normalize(normalMatrix * normal);
  
  lightPosition = (inverse(lightViewMatrix.value) * vec4(0, 0, 0, 1)).xyz;
  fragmentColor = vec3(1, 1, 1) * dot(worldNormal, normalize(lightPosition - worldPosition.xyz));
}




