#include "PBR_Shader.h"
#include "../../Serialization/Helpers/SerializerMacros.h"
#include "../../Serialization/Attributes/ColorAttribute.h"
#include "../../Serialization/Attributes/SliderAttribute.h"


namespace Jimara {
	struct PBR_Shader::Helpers {
		struct SettingsSerializer : public virtual Serialization::SerializerList::From<Settings> {
			const bool vec4Color;
			const bool hasAlphaThreshold;

			inline SettingsSerializer(bool useVec4Color, bool exposeAlphaThreshold) 
				: Serialization::ItemSerializer("Settings", "Material settings")
				, vec4Color(useVec4Color), hasAlphaThreshold(exposeAlphaThreshold) {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Settings* target)const final override {
				JIMARA_SERIALIZE_FIELDS(target, recordElement) {
					if (vec4Color)
						JIMARA_SERIALIZE_FIELD(target->albedo, "Albedo", "Main color of the material",
							Object::Instantiate<Serialization::ColorAttribute>());
					else {
						Vector3 color = target->albedo;
						JIMARA_SERIALIZE_FIELD(color, "Albedo", "Main color of the material",
							Object::Instantiate<Serialization::ColorAttribute>());
						target->albedo = Vector4(color, 1.0f);
					}
					JIMARA_SERIALIZE_FIELD(target->emission, "Emission", "Emission color",
						Object::Instantiate<Serialization::ColorAttribute>());
					JIMARA_SERIALIZE_FIELD(target->metalness, "Metalness", "Tells, if the material is metallic or dielectric",
						Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
					JIMARA_SERIALIZE_FIELD(target->roughness, "Roughness", "Tells, how rough the material surface is",
						Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
					if (hasAlphaThreshold)
						JIMARA_SERIALIZE_FIELD(target->alphaThreshold, "Alpha threshold", "Fragments with alpha less than this will be discarded",
							Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
					else target->alphaThreshold = 0.0f;
					JIMARA_SERIALIZE_FIELD(target->tiling, "Tiling", "Texture UV tiling");
					JIMARA_SERIALIZE_FIELD(target->offset, "Offset", "Texture UV offset");
				};
			}
		};
	};

	PBR_Shader::PBR_Shader(Graphics::GraphicsPipeline::BlendMode blendMode, const OS::Path& path, const void* settingsSerializer) 
		: Graphics::ShaderClass(path, blendMode), m_settingsSerializer(settingsSerializer) {}

	PBR_Shader* PBR_Shader::Opaque() {
		static const ConstantBufferSerializer<Settings> serializer(
			SETTINGS_NAME, Object::Instantiate<Helpers::SettingsSerializer>(false, false));
		static PBR_Shader instance(Graphics::GraphicsPipeline::BlendMode::REPLACE, "Jimara/Data/Materials/PBR/Jimara_PBR_Shader_Opaque", (const void*)&serializer);
		return &instance;
	}

	PBR_Shader* PBR_Shader::Cutout() {
		static const ConstantBufferSerializer<Settings> serializer(
			SETTINGS_NAME, Object::Instantiate<Helpers::SettingsSerializer>(true, true));
		static PBR_Shader instance(Graphics::GraphicsPipeline::BlendMode::REPLACE, "Jimara/Data/Materials/PBR/Jimara_PBR_Shader_Cutout", (const void*)&serializer);
		return &instance;
	}

	PBR_Shader* PBR_Shader::Transparent() {
		static const ConstantBufferSerializer<Settings> serializer(
			SETTINGS_NAME, Object::Instantiate<Helpers::SettingsSerializer>(true, true));
		static PBR_Shader instance(Graphics::GraphicsPipeline::BlendMode::ALPHA_BLEND, "Jimara/Data/Materials/PBR/Jimara_PBR_Shader_Transparent", (const void*)&serializer);
		return &instance;
	}

	Reference<const PBR_Shader::ConstantBufferBinding> PBR_Shader::DefaultConstantBufferBinding(
		const std::string_view& name, Graphics::GraphicsDevice* device)const {
		if (name == SETTINGS_NAME) return SharedConstantBufferBinding(Settings(), device);
		else return nullptr;
	}

	Reference<const PBR_Shader::TextureSamplerBinding> PBR_Shader::DefaultTextureSamplerBinding(
		const std::string_view& name, Graphics::GraphicsDevice* device)const {
		if (name == NORMAL_MAP_NAME)
			return ShaderClass::SharedTextureSamplerBinding(Vector4(0.5f, 0.5f, 1.0f, 1.0f), device);
		else return ShaderClass::DefaultTextureSamplerBinding(name, device);
	}

	void PBR_Shader::SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const {
		{
			((const ConstantBufferSerializer<Settings>*)m_settingsSerializer)->Serialize(reportField, bindings);
		}
		{
			static const TextureSamplerSerializer serializer(BASE_COLOR_NAME, "Base Color", "Base albedo color");
			reportField(serializer.Serialize(bindings));
		}
		{
			static const TextureSamplerSerializer serializer(NORMAL_MAP_NAME, "Normal map", 
				"Tangent space normal map ");
			reportField(serializer.Serialize(bindings));
		}
#define PBR_Shader_SerializeBindings_MRO_HINT(channel) \
	"(note that only " channel " channel will be used by the shader; you can compress memory by storing RGB as metalness, roughness and occlusion, respectively)"
		{
			static const TextureSamplerSerializer serializer(METALNESS_MAP_NAME, "Metalness", 
				"Metalness map " PBR_Shader_SerializeBindings_MRO_HINT("R"));
			reportField(serializer.Serialize(bindings));
		}
		{
			static const TextureSamplerSerializer serializer(ROUGHNESS_MAP_NAME, "Roughness", 
				"Roughness map "  PBR_Shader_SerializeBindings_MRO_HINT("G"));
			reportField(serializer.Serialize(bindings));
		}
		{
			static const TextureSamplerSerializer serializer(OCCLUSION_MAP_NAME, "Occlusion", 
				"Occlusion map "  PBR_Shader_SerializeBindings_MRO_HINT("B"));
			reportField(serializer.Serialize(bindings));
		}
		{
			static const TextureSamplerSerializer serializer(EMISSION_MAP_NAME, "Emission", "Emission color map");
			reportField(serializer.Serialize(bindings));
		}
#undef PBR_Shader_SerializeBindings_MRO_HINT
	}
}
