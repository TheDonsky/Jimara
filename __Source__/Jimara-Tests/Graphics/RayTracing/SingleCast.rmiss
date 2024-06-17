#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec4 color;

void main() {
	const vec3 dir = gl_WorldRayDirectionEXT;
	color = vec4(abs(dir.x), abs(dir.y), abs(dir.z), 1.0);
}
