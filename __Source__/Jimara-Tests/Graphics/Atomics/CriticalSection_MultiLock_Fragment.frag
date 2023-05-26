#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_control_flow_attributes : require 
#include "Graphics/Memory/Jimara_Atomics.glh"

struct PixelState {
	uint lock;
	uint count;
};

layout(set = 0, binding = 0) buffer States {
	PixelState[] forPixel;
} states;

layout(location = 0) in vec2 fragPosition;

void main() { 
	const vec2 pixelSize = vec2(dFdx(fragPosition).x, dFdy(fragPosition).y);
	const ivec2 pixelCount = ivec2(abs(int(1.0 / pixelSize.x + 0.5)), abs(int(1.0 / pixelSize.y + 0.5)));
	const ivec2 pixelIndex = ivec2(int(fragPosition.x * pixelCount.x), int(fragPosition.y * pixelCount.y));
	if (pixelIndex.x < 0 || pixelIndex.x >= pixelCount.x ||
		pixelIndex.y < 0 || pixelIndex.y >= pixelCount.y) discard;

	const uint index = pixelIndex.y * pixelCount.x + pixelIndex.x;
	Jimara_StartCriticalSection(states.forPixel[index].lock);
	states.forPixel[index].count++;
	Jimara_EndCriticalSection(states.forPixel[index].lock);
}
