#version 460
#extension GL_EXT_ray_tracing : enable

struct Payload {
	vec3 color;
	vec3 reflectionDir;
	vec3 hitPoint;
};
layout(location = 0) rayPayloadInEXT Payload prd;

void main() {
	prd.color = mix(
		vec3(0.25, 0.75, 0.25), vec3(0.5, 0.5, 1.0), 
		gl_WorldRayDirectionEXT.y * 0.5 + 0.5);
}
