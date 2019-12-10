#version 450

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 fragmentColor;

layout(set = 0, binding = 0) uniform PerFrameData {
  mat4 matrix;
} perFrameData;

layout(set = 1, binding = 1) uniform DrawCallData {
  mat4 matrix;
} drawCallData;

void main() {
  gl_Position = perFrameData.matrix * drawCallData.matrix * vec4(pos, 1.0);
  
  fragmentColor = vec3(1, 1, 1) * (1 - gl_Position.z*0.2);
}
