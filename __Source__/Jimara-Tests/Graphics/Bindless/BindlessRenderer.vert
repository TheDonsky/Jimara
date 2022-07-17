#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

struct Vertex {
    vec2 position;
    vec2 uv;
};

layout (set = 1, binding = 0) buffer VertexBuffers {
    Vertex[] verts;
} vertexBuffers[];

layout(set = 2, binding = 1) uniform VertexInfo {
    vec2 offset;
    float scale;
    uint vertexBufferIndex;
} vertexInfo;

layout(location = 0) out vec2 fragTexturePosition;

void main() {
    Vertex vert = vertexBuffers[vertexInfo.vertexBufferIndex].verts[gl_VertexIndex];
    fragTexturePosition = vert.uv;
    vec2 position = (vert.position * vertexInfo.scale) + vertexInfo.offset;
    gl_Position = vec4(position.x, position.y, 0.0, 1.0);
}
