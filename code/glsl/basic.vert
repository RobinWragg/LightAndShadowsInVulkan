#version 450

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 fragmentColor;

layout(binding = 0) uniform PerFrameData {
  mat4 matrix;
} perFrameData;

void main() {
  gl_Position = perFrameData.matrix * vec4(pos, 1.0);
  
  fragmentColor = vec3(1, 0, 1) * (1 - gl_Position.z);
}
