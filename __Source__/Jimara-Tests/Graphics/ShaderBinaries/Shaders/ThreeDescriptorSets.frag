#version 450

layout(set = 1, binding = 1) uniform sampler2D textureSampler_1_1;

layout(set = 1, binding = 10) uniform sampler2D textureSampler_1_10;

layout(set = 1, binding = 0) buffer StructuredBuffer_1_0 {
	int value[];
} structuredBuffer_1_0;

layout(set = 1, binding = 8) uniform ConstantBuffer_1_8 {
	float value;
} constantBuffer_1_8;

layout(set = 2, binding = 1) uniform sampler2D textureSampler_2_1;

layout(set = 2, binding = 4) buffer StructuredBuffer_2_4 {
	int value[];
} structuredBuffer_2_4;

layout(set = 2, binding = 3) uniform ConstantBuffer_2_3 {
	float value;
} constantBuffer_2_3;

void main() {}
