#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec4 color;


void main() {
	const mat4x3 pose = gl_ObjectToWorldEXT;
	const vec3 position = pose[3];
	const float time = gl_HitTEXT;
	const vec3 hitPoint = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * time;
	const vec3 normal = normalize(hitPoint - position);
	const vec3 normalColor = normal * 0.5 + 0.5;
	color = vec4(normalColor, 1.0);
}
