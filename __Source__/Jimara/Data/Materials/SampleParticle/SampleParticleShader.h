#pragma once
#include "../../Materials/Material.h"


namespace Jimara {
	/// <summary> Let the registry know about the shader </summary>
	JIMARA_REGISTER_TYPE(Jimara::SampleParticleShader);

	/// <summary>
	/// Tools for test shader for particles
	/// </summary>
	class JIMARA_API SampleParticleShader final {
	public:
		/// <summary> Path to the sample particle shader </summary>
		static const OS::Path PATH;

		/// <summary> Base Color parameter (vec4) name </summary>
		static const constexpr std::string_view BASE_COLOR_NAME = "baseColor";

		/// <summary> Albedo parameter (sampler2D) name </summary>
		static const constexpr std::string_view ALBEDO_NAME = "albedo";

		/// <summary> Alpha-Threshold parameter (sampler2D) name </summary>
		static const constexpr std::string_view ALPHA_THRESHOLD_NAME = "alphaThreshold";

	private:
		// Constructor is inaccessible...
		inline SampleParticleShader() {}
	};
}

