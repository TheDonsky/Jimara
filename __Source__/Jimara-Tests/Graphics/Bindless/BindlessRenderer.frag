#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 0) uniform sampler2D textures[];

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform TextureIndex {
    uint index;
} textureIndex;

layout(location = 0) in vec2 fragTexturePosition;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(textures[textureIndex.index], fragTexturePosition);
}
