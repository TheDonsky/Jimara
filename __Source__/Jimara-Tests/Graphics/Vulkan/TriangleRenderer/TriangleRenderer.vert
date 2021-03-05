#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera {
    mat4 cameraTransform;
} camera;

layout(set = 1, binding = 1) uniform Constants {
    float scale;
} constants;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexturePosition;
layout(location = 2) out vec3 fragPosition;

layout(location = 0) in vec2 vertPosition;

layout(location = 1) in vec2 vertOffset;

vec3 colors[6] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    vec2 pos = (vertPosition + vertOffset) * (1.0f + constants.scale);
    vec4 position = vec4(pos.x, 0.15, pos.y, 1.0);
    gl_Position = position * camera.cameraTransform;
    fragColor = colors[gl_VertexIndex];
    fragTexturePosition = position.xz;
    fragPosition = position.xyz;
}
