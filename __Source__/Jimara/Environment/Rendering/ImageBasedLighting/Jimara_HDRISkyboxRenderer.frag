#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_control_flow_attributes : require 
#include "HDRI.glh"

layout(set = 0, binding = 0) uniform Settings {
	vec4 color;
} settings;
layout(set = 0, binding = 1) uniform sampler2D hdri;

layout(location = 0) in vec3 direction;
layout(location = 0) out vec4 outColor;

void main() {
	const vec2 hdriSize = textureSize(hdri, 0);
	const vec2 uv = Jimara_HDRI_UV(normalize(direction));
	const vec2 duvDx = dFdx(uv) * hdriSize;
	const vec2 duvDy = dFdx(uv) * hdriSize;
	const float delta = max(
		max(min(duvDx.x, 1.0 - duvDx.x), min(duvDy.x, 1.0 - duvDy.x)),
		max(min(duvDx.y, 1.0 - duvDx.y), min(duvDy.y, 1.0 - duvDy.y)));
	outColor = textureLod(hdri, uv, log2(delta)) * settings.color;
}
