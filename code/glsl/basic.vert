#version 450

layout(location = 0) in float posX;
layout(location = 1) in float posY;
layout(location = 2) in float posZ;
layout(location = 3) in float brightness;

layout(location = 0) out vec3 fragmentColor;

void main() {
	gl_PointSize = 2;
	gl_Position = vec4(posX, posY, posZ, 1.0);

	// Set the water shade based on vertBrightness and dim the fragment based on the particle's depth (posZ).
	float depthDarkening = (1 - posZ*posZ) * 1.1;
	fragmentColor = vec3(brightness, brightness, 1) * depthDarkening;
}