#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec2 vertPosition;
layout(location = 1) in vec4 vertColor;

layout(location = 0) out vec4 fragColor;

void main() {
	gl_Position = vec4(vertPosition, 0.0, 1.0);
	fragColor = vertColor;
}
