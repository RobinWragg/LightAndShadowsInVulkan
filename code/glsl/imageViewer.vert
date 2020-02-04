#version 450

layout(location = 0) in vec3 position;
layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec3 fragmentColor;

layout(push_constant) uniform PushConstant {
  mat4 matrix;
} pushConstant;

void main() {
  gl_Position = pushConstant.matrix * vec4(position, 1.0);
  texCoord.xy = position.xy;
  fragmentColor = vec3(1, 1, 1);
}




