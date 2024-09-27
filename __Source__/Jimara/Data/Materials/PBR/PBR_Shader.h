#pragma once
#include "../../Materials/Material.h"


namespace Jimara {
	/// <summary> Let the registry know about the shader </summary>
	JIMARA_REGISTER_TYPE(Jimara::PBR_Shader);

	/// <summary>
	/// PBR Shader tools
	/// </summary>
	class JIMARA_API PBR_Shader final {
	public:
		/// <summary> Color-Only opaque PBR Shader path (only has color, metalness and roughness parameters) </summary>
		static const OS::Path COLOR;

		/// <summary> Standard Opaque PBR Shader path </summary>
		static const OS::Path OPAQUE_PATH;

		/// <summary> Standard Cutout PBR Shader path </summary>
		static const OS::Path CUTOUT_PATH;

		/// <summary> Standard Transparent PBR Shader path </summary>
		static const OS::Path TRANSPARENT_PATH;

		/// <summary>
		/// Searches for the opaque shader
		/// </summary>
		/// <param name="shaders"> Shader set </param>
		/// <returns> Opaque shader definition, if found </returns>
		inline static const Material::LitShader* Opaque(const Material::LitShaderSet* shaders) { return shaders->FindByPath(OPAQUE_PATH); }

		/// <summary>
		/// Searches for the cutout shader
		/// </summary>
		/// <param name="shaders"> Shader set </param>
		/// <returns> Cutout shader definition, if found </returns>
		inline static const Material::LitShader* Cutout(const Material::LitShaderSet* shaders) { return shaders->FindByPath(CUTOUT_PATH); }

		// <summary>
		/// Searches for the transparent shader
		/// </summary>
		/// <param name="shaders"> Shader set </param>
		/// <returns> Transparent shader definition, if found </returns>
		inline static const Material::LitShader* Transparent(const Material::LitShaderSet* shaders) { return shaders->FindByPath(TRANSPARENT_PATH); }

		/// <summary> Base color property name </summary>
		static const constexpr std::string_view ALBEDO_NAME = "albedo";

		/// <summary> Base color sampler property name </summary>
		static const constexpr std::string_view BASE_COLOR_NAME = "baseColor";

		/// <summary> Normal map sampler name </summary>
		static const constexpr std::string_view NORMAL_MAP_NAME = "normalMap";

		/// <summary> Metalness property name </summary>
		static const constexpr std::string_view METALNESS_NAME = "metalness";

		/// <summary> Metalness map sampler name (Only R channel is read by the shader; metalness, roughness and occlusion maps can be merged) </summary>
		static const constexpr std::string_view METALNESS_MAP_NAME = "metalnessMap";

		/// <summary> Roughness property name </summary>
		static const constexpr std::string_view ROUGHNESS_NAME = "roughness";

		/// <summary> Roughness map sampler name (Only G channel is read by the shader; metalness, roughness and occlusion maps can be merged) </summary>
		static const constexpr std::string_view ROUGHNESS_MAP_NAME = "roughnessMap";

		/// <summary> Occlusion map sampler name (Only B channel is read by the shader; metalness, roughness and occlusion maps can be merged) </summary>
		static const constexpr std::string_view OCCLUSION_MAP_NAME = "occlusionMap";

		/// <summary> Emission color property name </summary>
		static const constexpr std::string_view EMISSION_NAME = "emission";

		/// <summary> Emissive color sampler name </summary>
		static const constexpr std::string_view EMISSION_MAP_NAME = "emissionMap";

		/// <summary> Alpha-Threshold property name </summary>
		static const constexpr std::string_view ALPHA_THRESHOLD_NAME = "alphaThreshold";

		/// <summary> UV Tiling property name </summary>
		static const constexpr std::string_view TILING_NAME = "tiling";
		
		/// <summary> UV Offset property name </summary>
		static const constexpr std::string_view OFFSET_NAME = "offset";

	private:
		// Constructor is inaccessible...
		inline PBR_Shader() {}
	};
}
