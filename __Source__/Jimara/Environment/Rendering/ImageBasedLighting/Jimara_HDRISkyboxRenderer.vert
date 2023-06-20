#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 direction;

layout(location = 0) out vec3 fragmentDirection;

void main() {
	fragmentDirection = direction;
	gl_Position = vec4(position.x, position.y, 0.0, 1.0);
}
