#version 450
#include "Jimara_ImageOverlayRenderer.frag.glh"

layout(set = 0, binding = 1) uniform sampler2D image;
vec4 SampleTexture() {
	return texture(image, uv);
}