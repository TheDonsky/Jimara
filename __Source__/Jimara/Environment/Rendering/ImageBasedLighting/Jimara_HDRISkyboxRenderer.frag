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
	outColor = Jimara_HDRI_SampleTexture(hdri, normalize(direction)) * settings.color;
}
