#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Light {
	vec3 position;
	vec3 color;
};

layout(std430, set = 0, binding = 1) buffer Lights {
	Light lights[];
};

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexturePosition;
layout(location = 2) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 totalLight = vec3(0, 0, 0);
	for (int i = 0; i < lights.length(); i++) {
		Light light = lights[i];
		vec3 diff = (fragPosition - light.position);
		totalLight += light.color / dot(diff, diff);
	}
	outColor = clamp(vec4(fragColor, 1.0) * texture(texSampler, fragTexturePosition) * vec4(totalLight, 1.0), 0.0, 1.0);
}
