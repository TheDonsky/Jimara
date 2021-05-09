#version 450

layout(set = 0, binding = 3) uniform ConstantBuffer_0_3 {
	float value;
} constantBuffer_0_3;

layout(set = 0, binding = 7) buffer StructuredBuffer_0_7 {
	float value[];
} structuredBuffer_0_7;

layout(set = 0, binding = 2) uniform sampler2D textureSampler_0_2;

layout(set = 0, binding = 5) buffer StructuredBuffer_0_5 {
	int value[];
} structuredBuffer_0_5;


layout(set = 1, binding = 1) uniform sampler2D textureSampler_1_1;

layout(set = 1, binding = 2) uniform sampler2D textureSampler_1_2;

layout(set = 1, binding = 0) buffer StructuredBuffer_1_0 {
	int value[];
} structuredBuffer_1_0;

layout(set = 1, binding = 5) uniform ConstantBuffer_1_5 {
	float value;
} constantBuffer_1_5;

layout(set = 1, binding = 8) uniform ConstantBuffer_1_8 {
	float value;
} constantBuffer_1_8;


void main() {}
