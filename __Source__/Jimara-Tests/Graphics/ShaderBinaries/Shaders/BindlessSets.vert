#version 450

layout (set = 0, binding = 0) uniform sampler2D textures[];

struct StructuredTypeA {
    int a;
    int b;
};

layout (set = 1, binding = 0) buffer StructuredTypeAliasA {
    StructuredTypeA elems[];
} structuredTypeAliasA[];

struct StructuredTypeB {
    float x;
    float y;
};

layout (set = 1, binding = 0) buffer StructuredTypeAliasB {
    StructuredTypeA elems[];
} structuredTypeAliasB[];

layout (set = 2, binding = 1) uniform ConstantBuffer {
    int someValue;
} constantBuffer[];


void main() {}
