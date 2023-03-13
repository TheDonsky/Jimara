#include "Jimara_ImageOverlayRenderer_Settings.glh"

layout(location = 0) in vec4 vertPosAndUV;
layout(location = 0) out vec2 uv;

void main() {
	gl_Position = vec4(vertPosAndUV.xy * settings.scale + settings.offset, 0.5, 1.0);
	uv = vertPosAndUV.zw * settings.uvScale + settings.uvOffset;
}
