#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec4 color;

void main() {
	color.z += color.x * color.y;
	color *= vec4(1.0, 0.0, 1.0, 1.0);
}
