#pragma once
#include "../../Material.h"


namespace Jimara {
	/// <summary> Let the registry know about the shader </summary>
	JIMARA_REGISTER_TYPE(Jimara::PBR_Shader);

	class JIMARA_API PBR_Shader : public virtual Graphics::ShaderClass {
	public:
		/// <summary> Ppaque version </summary>
		static PBR_Shader* Opaque();

		/// <summary> Cutout version </summary>
		static PBR_Shader* Cutout();

		/// <summary> Transparent version </summary>
		static PBR_Shader* Transparent();

		/// <summary> PBR Shader settings buffer content </summary>
		struct JIMARA_API Settings {
			/// <summary> Main color (multiplier) of the material (alpha ignored for opaque materials) </summary>
			alignas(16) Vector4 albedo = Vector4(1.0f);

			/// <summary> Emission color multiplier </summary>
			alignas(16) Vector3 emission = Vector3(0.0f);

			/// <summary> PBR Metalness </summary>
			alignas(4) float metalness = 0.1f;

			/// <summary> PBR Roughness </summary>
			alignas(4) float roughness = 0.5f;

			/// <summary> Fragments with alpha less than this will be discarded (ignored by opaque materials) </summary>
			alignas(4) float alphaThreshold = 0.25f;

			/// <summary> Texture UV tiling </summary>
			alignas(8) Vector2 tiling = Vector2(1.0f);

			/// <summary> Texture UV offset </summary>
			alignas(8) Vector2 offset = Vector2(0.0f);
		};

		/// <summary> Settings cbuffer binding name </summary>
		static const constexpr std::string_view SETTINGS_NAME = "settings";

		/// <summary> Base color sampler binding name </summary>
		static const constexpr std::string_view BASE_COLOR_NAME = "baseColor";

		/// <summary> Normal map sampler binding name </summary>
		static const constexpr std::string_view NORMAL_MAP_NAME = "normalMap";

		/// <summary> Metalness map sampler binding name (Only R channel is read by the shader; metalness, roughness and occlusion maps can be merged) </summary>
		static const constexpr std::string_view METALNESS_MAP_NAME = "metalnessMap";

		/// <summary> Roughness map sampler binding name (Only G channel is read by the shader; metalness, roughness and occlusion maps can be merged) </summary>
		static const constexpr std::string_view ROUGHNESS_MAP_NAME = "roughnessMap";

		/// <summary> Occlusion map sampler binding name (Only B channel is read by the shader; metalness, roughness and occlusion maps can be merged) </summary>
		static const constexpr std::string_view OCCLUSION_MAP_NAME = "occlusionMap";

		/// <summary> Emissive color sampler binding name </summary>
		static const constexpr std::string_view EMISSION_MAP_NAME = "emissionMap";

		/// <summary>
		/// Gets default constant buffer binding per device
		/// </summary>
		/// <param name="name"> Binding name </param>
		/// <param name="device"> Graphics device </param>
		/// <returns> Constant buffer binding instance for the device </returns>
		virtual Reference<const ConstantBufferBinding> DefaultConstantBufferBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const override;

		/// <summary>
		/// Gets default texture per device
		/// </summary>
		/// <param name="name"> Binding name </param>
		/// <param name="device"> Graphics device </param>
		/// <returns> Texture binding instance for the device </returns>
		virtual Reference<const TextureSamplerBinding> DefaultTextureSamplerBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const override;

		/// <summary>
		/// Serializes shader bindings (like textures and constants)
		/// </summary>
		/// <param name="reportField"> 
		/// Each binding can be represented as an arbitrary SerializedObject (possibly with some nested entries); 
		/// Serialization scheme can use these to control binding set.
		/// </param>
		/// <param name="bindings"> Binding set to serialize </param>
		virtual void SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const override;

	private:
		const void* const m_settingsSerializer;

		// Constructor is private...
		PBR_Shader(Graphics::GraphicsPipeline::BlendMode blendMode, const OS::Path& path, const void* settingsSerializer);

		// Some private stuff is here...
		struct Helpers;
	};

	// Type id details
	template<> inline void TypeIdDetails::GetParentTypesOf<PBR_Shader>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Graphics::ShaderClass>()); }
	template<> inline void TypeIdDetails::GetTypeAttributesOf<PBR_Shader>(const Callback<const Object*>& reportAttribute) { 
		reportAttribute(PBR_Shader::Opaque());
		reportAttribute(PBR_Shader::Cutout());
		reportAttribute(PBR_Shader::Transparent());
	}
}
