#version 450
#include "Jimara_ImageOverlayRenderer.frag.glh"

layout(set = 0, binding = 1) uniform sampler2DMS image;

vec4 SampleTexture() {
	const ivec2 coord = ivec2(uv * (settings.imageSize - 1) + 0.5);
	vec4 sum = vec4(0, 0, 0, 0);
	for (int i = 0; i < settings.sampleCount; i++)
		sum += texelFetch(image, coord, i);
	return sum / settings.sampleCount;
}
