#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera {
    mat4 cameraTransform;
} camera;

layout(location = 0) in vec3 vertPosition;
layout(location = 1) in vec3 vertNormal;
layout(location = 2) in vec2 vertUV;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;


void main() {
	float scale = 0.75;
	gl_Position = camera.cameraTransform * vec4(vertPosition * scale, 1.0f);
	fragPosition = vertPosition * scale;
	fragNormal = vertNormal;
	fragUV = vertUV;
}
