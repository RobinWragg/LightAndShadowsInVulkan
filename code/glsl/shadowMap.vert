#version 450

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec4 vertPosInView;

layout(set = 0, binding = 0) uniform DrawCall {
  mat4 worldMatrix;
} drawCall;

layout(set = 1, binding = 0) uniform Matrices {
  mat4 view;
  mat4 proj;
} matrices;

void main() {
  vec4 viewOffsetTODO = vec4(0, 0, 0, 0);
  vertPosInView = matrices.view * drawCall.worldMatrix * vec4(vertPos, 1.0) + viewOffsetTODO;
  gl_Position = matrices.proj * vertPosInView;
}




