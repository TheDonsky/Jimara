#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_ray_query : enable

layout(set = 0, binding = 0) uniform Constants {
	vec3 right;
	vec3 up;
	vec3 forward;
	vec3 position;
} constants;

layout (set = 0, binding = 1) uniform accelerationStructureEXT accelerationStructure;

layout(location = 0) in vec3 rayDirection;

layout(location = 0) out vec4 color;


void main() {
	const vec3 dir = normalize(rayDirection);
	const float minT = 0.0001;
	const float maxT = 10000.0;

	rayQueryEXT query;
	rayQueryInitializeEXT(
		query, accelerationStructure,
		gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
		0xFF, constants.position, minT, dir, maxT);
	while(rayQueryProceedEXT(query)) {}
	if (rayQueryGetIntersectionTypeEXT(query, true) != 0) {
		const mat4x3 pose = rayQueryGetIntersectionObjectToWorldEXT(query, true);
		const vec3 position = pose[3];
		const float time = rayQueryGetIntersectionTEXT(query, true);
		const vec3 hitPoint = constants.position + dir * time;
		const vec3 normal = normalize(hitPoint - position);
		const vec3 normalColor = normal * 0.5 + 0.5;
		color = vec4(normalColor, 1.0);
	}
	else color = vec4(abs(dir.x), abs(dir.y), abs(dir.z), 1.0);
}
