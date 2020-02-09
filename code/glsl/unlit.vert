#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 fragmentColor;

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
  gl_Position = projMatrix.value * viewMatrix.value * pushConstant.model * vec4(position, 1.0);
  fragmentColor = vec3(1, 1, 1);
}




