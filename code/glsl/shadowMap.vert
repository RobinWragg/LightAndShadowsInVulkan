#version 450

layout(location = 0) in vec3 vertPosition;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec4 worldPosition;

layout(set = 0, binding = 0) uniform ViewMatrix {
  mat4 value;
} viewMatrix;

layout(set = 1, binding = 0) uniform ProjMatrix {
  mat4 value;
} projMatrix;

layout(push_constant) uniform PushConstant {
  mat4 model;
} pushConstant;

void main() {
  worldPosition = pushConstant.model * vec4(vertPosition, 1.0);
  gl_Position = projMatrix.value * viewMatrix.value * worldPosition;
}




