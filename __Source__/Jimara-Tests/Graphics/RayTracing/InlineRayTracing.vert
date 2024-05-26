#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Constants {
	vec3 right;
	vec3 up;
	vec3 forward;
	vec3 position;
} constants;

layout(location = 0) out vec3 rayDirection;

vec2 verts[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(1.0, 1.0),
    vec2(-1.0, 1.0),

    vec2(-1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(1.0, 1.0)
);

void main() {
	const vec2 screenPosition = verts[gl_VertexIndex];
	gl_Position = vec4(screenPosition.x, screenPosition.y, 0.0, 1.0);
	rayDirection = screenPosition.x * constants.right + screenPosition.y * constants.up + constants.forward;
}
