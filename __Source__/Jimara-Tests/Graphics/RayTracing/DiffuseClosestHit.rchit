#version 460
#extension GL_EXT_ray_tracing : enable

struct Vertex {
	vec3 position;
	float pad0;
	vec3 normal;
	float pad1;
	vec2 uv;
	vec2 pad2;
};
layout (set = 0, binding = 3) readonly buffer Vertices { 
	Vertex data[]; 
} vertices;
layout (set = 0, binding = 4) readonly buffer Indices { 
	uint data[]; 
} indices;

hitAttributeEXT vec2 attribs;
layout(location = 0) rayPayloadInEXT vec4 color;

void main() {
	const uvec3 tri = uvec3(
		indices.data[gl_PrimitiveID * 3 + 0],
		indices.data[gl_PrimitiveID * 3 + 1],
		indices.data[gl_PrimitiveID * 3 + 2]);
	const Vertex a = vertices.data[tri.x];
	const Vertex b = vertices.data[tri.y];
	const Vertex c = vertices.data[tri.z];

	const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

	Vertex v;
	v.position =
		barycentrics.x * a.position +
		barycentrics.y * b.position + 
		barycentrics.z * c.position;
	v.normal = normalize(
		barycentrics.x * a.normal +
		barycentrics.y * b.normal + 
		barycentrics.z * c.normal);
	v.uv =
		barycentrics.x * a.uv +
		barycentrics.y * b.uv + 
		barycentrics.z * c.uv;


	const mat4x3 pose = gl_ObjectToWorldEXT;

	mat4x4 transform;
	transform[0] = vec4(pose[0], 0.0);
	transform[1] = vec4(pose[1], 0.0);
	transform[2] = vec4(pose[2], 0.0);
	transform[3] = vec4(pose[3], 1.0);

	color = vec4(vec3(dot(
		normalize((transform * vec4(v.normal, 0.0)).xyz), 
		-gl_WorldRayDirectionEXT)), 1.0);
}
