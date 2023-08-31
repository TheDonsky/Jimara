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

layout(set = 0, binding = 3) uniform image2D colorAttachment;

void main() {
	const ivec2 pixelIndex = ivec2(gl_FragCoord.xy);
	if (pixelIndex.x >= settings.frameBufferSize.x || pixelIndex.y >= settings.frameBufferSize.y)
		discard;

	const uint pixelBufferIndex = pixelIndex.y * settings.frameBufferSize.x + pixelIndex.x;
	const PixelState pixelInfo = resultBufferPixels.state[pixelBufferIndex];
	if (pixelInfo.fragmentCount <= 0)
		discard;

	vec4 color = imageLoad(colorAttachment, pixelIndex);
	color.rgb *= color.a;

	const uint fragmentStartIndex = pixelBufferIndex * settings.fragsPerPixel;
	uint fragmentIndex = (fragmentStartIndex + pixelInfo.fragmentCount);
	while (fragmentIndex > fragmentStartIndex) {
		fragmentIndex--;
		const FragmentInfo fragment = fragmentData.fragments[fragmentIndex];
		vec4 fragColor;
		fragColor.rg = unpackHalf2x16(fragment.packedRG);
		fragColor.ba = unpackHalf2x16(fragment.packedBA);
		color.rgb = color.rgb * fragColor.a + fragColor.rgb;
		color.a = 1 - (1 - color.a) * fragColor.a;
	}

	if (color.a > 0.000001)
		color.rgb /= color.a;
	
	imageStore(colorAttachment, pixelIndex, color);
	gl_FragDepth = fragmentData.fragments[fragmentStartIndex].depth;
}
