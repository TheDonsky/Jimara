#version 450

layout(location = 0) in float floatInput0;
layout(location = 1) in float floatInput1;
layout(location = 4) in vec2 vec2Input;
layout(location = 5) in vec3 vec3Input;
layout(location = 6) in vec4 vec4Input0;
layout(location = 7) in vec4 vec4Input1;

layout(location = 8) in int intInput;
layout(location = 9) in ivec2 ivec2Input;
layout(location = 10) in ivec3 ivec3Input;
layout(location = 11) in ivec4 ivec4Input;

layout(location = 16) in uint uintInput;
layout(location = 17) in uvec2 uvec2Input;
layout(location = 18) in uvec3 uvec3Input;
layout(location = 19) in uvec4 uvec4Input;

layout(location = 24) in mat2 mat2Input;
layout(location = 28) in mat3 mat3Input;
layout(location = 32) in mat4 mat4Input;

layout(location = 38) out vec3 pos;

void main() {}
