#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vertPosition;

layout(location = 0) out vec2 fragPosition;

void main() { 
	gl_Position = vec4(vertPosition * 2.0 - 1.0, 0.0, 1.0);
	fragPosition = vertPosition;
}
