#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 fragmentColor;

layout(set = 0, binding = 0) uniform DrawCall {
  mat4 worldMatrix;
} drawCall;

layout(set = 2, binding = 0) uniform Matrices {
  mat4 view;
  mat4 proj;
} matrices;

void main() {
  gl_Position = matrices.proj * matrices.view * drawCall.worldMatrix * vec4(position, 1.0);
  fragmentColor = vec3(1, 1, 1);
}




