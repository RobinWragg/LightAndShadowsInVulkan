#version 450

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 fragmentColor;

void main() {
	gl_Position = vec4(pos, 1.0);
	fragmentColor = vec3(1, 0, 1);
}