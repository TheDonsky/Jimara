#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_shader_image_load_formatted : require

layout(set = 0, binding = 0) uniform Settings {
	uvec2 frameBufferSize;
	uint fragsPerPixel;
} settings;

struct PixelState {
	uint lock;
	uint fragmentCount;
};
layout(set = 0, binding = 1) buffer readonly ResultBufferPixels {
	PixelState state[];
} resultBufferPixels;

struct FragmentInfo {
	float depth;
	uint packedRG; // Color is stored premultiplied
	uint packedBA; // Instead of alpha, we store transmittance (1 - a) for transparent and 1 for additive
};
layout(set = 0, binding = 2) buffer readonly FragmentData {
	FragmentInfo fragments[];
} fragmentData;

void main() {
	const ivec2 pixelIndex = ivec2(gl_FragCoord.xy);
	if (pixelIndex.x >= settings.frameBufferSize.x || pixelIndex.y >= settings.frameBufferSize.y)
		discard;

	const uint pixelBufferIndex = pixelIndex.y * settings.frameBufferSize.x + pixelIndex.x;
	const PixelState pixelInfo = resultBufferPixels.state[pixelBufferIndex];
	if (pixelInfo.fragmentCount <= 0)
		discard;

	const uint fragmentIndex = pixelBufferIndex * settings.fragsPerPixel;
	gl_FragDepth = fragmentData.fragments[fragmentIndex].depth;
}
