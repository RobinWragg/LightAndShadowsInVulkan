#version 450

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 fragmentColor;

void main() {
	gl_Position = vec4(pos, 1.0);

	// Set the water shade based on vertBrightness and dim the fragment based on the particle's depth (posZ).
	fragmentColor = vec3(pos.x+0.5, 0, 1);
}
