#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#include "HDRI.glh"

layout(set = 0, binding = 0) uniform sampler2D hdri;

layout(location = 0) in vec3 direction;
layout(location = 0) out vec4 outColor;

void main() {
	outColor = Jimara_HDRI_SampleTexture(hdri, normalize(direction));
}
