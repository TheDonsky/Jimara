#version 460
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_ray_tracing : enable

struct Vertex {
	vec3 position;
	float pad0;
	vec3 normal;
	float pad1;
	vec2 uv;
	vec2 pad2;
};

layout(buffer_reference) buffer Vertices { Vertex data[]; };
layout(buffer_reference) buffer Indices { uint data[]; };

struct BufferRefs {
	uint64_t verts;
	uint64_t indices;
};
layout (set = 0, binding = 3) readonly buffer ObjectBuffers { 
	BufferRefs data[]; 
} objectBuffers;

hitAttributeEXT vec2 attribs;
struct Payload {
	vec3 color;
	vec3 reflectionDir;
	vec3 hitPoint;
};
layout(location = 0) rayPayloadInEXT Payload prd;

void main() {
	const BufferRefs bufferRefs = objectBuffers.data[gl_InstanceID];
	Vertices vertices = Vertices(bufferRefs.verts);
	Indices indices = Indices(bufferRefs.indices);

	const uvec3 tri = uvec3(
		indices.data[gl_PrimitiveID * 3 + 0],
		indices.data[gl_PrimitiveID * 3 + 1],
		indices.data[gl_PrimitiveID * 3 + 2]);
	const Vertex a = vertices.data[tri.x];
	const Vertex b = vertices.data[tri.y];
	const Vertex c = vertices.data[tri.z];

	const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

	const mat4x3 pose = gl_ObjectToWorldEXT;

	mat4x4 transform;
	transform[0] = vec4(pose[0], 0.0);
	transform[1] = vec4(pose[1], 0.0);
	transform[2] = vec4(pose[2], 0.0);
	transform[3] = vec4(pose[3], 1.0);

	const vec3 position = (transform * vec4(
		barycentrics.x * a.position +
		barycentrics.y * b.position + 
		barycentrics.z * c.position, 1.0)).xyz;
	const vec3 normal = normalize((transform * vec4(
		barycentrics.x * a.normal +
		barycentrics.y * b.normal + 
		barycentrics.z * c.normal, 0.0)).xyz);

	const vec3 rayDir = -gl_WorldRayDirectionEXT;
	const float cosine = dot(normal, rayDir);
	const vec3 diff = rayDir - normal * cosine;
	const vec3 reflDir = rayDir - 2.0 * diff;

	prd.color = vec3(cosine);
	prd.reflectionDir = 0.75 * cosine * reflDir;
	prd.hitPoint = position;
}
