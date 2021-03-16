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

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

vec3 Color(vec3 direction, vec3 color) {
	float tangent = dot(-direction, normalize(fragNormal));
	if (tangent < 0.0) return vec3(0.0, 0.0, 0.0);
	return color * tangent;
}

void main() {
	vec3 result = vec3(0.0, 0.0, 0.0);
	for (int i = 0; i < lights.length(); i++) {
		Light light = lights[i];
		vec3 diff = (fragPosition - light.position);
		float diffSqrMagn = dot(diff, diff);
		if (diffSqrMagn <= 0.0001) continue;
		result += Color(diff / sqrt(diffSqrMagn), light.color / diffSqrMagn);
	}
	outColor = clamp(vec4(result, 1.0) * texture(texSampler, fragUV), 0.0, 1.0);
}
