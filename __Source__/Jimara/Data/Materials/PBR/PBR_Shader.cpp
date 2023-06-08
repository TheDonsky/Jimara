#include "PBR_Shader.h"
#include "../../Serialization/Helpers/SerializerMacros.h"
#include "../../Serialization/Attributes/ColorAttribute.h"
#include "../../Serialization/Attributes/SliderAttribute.h"


namespace Jimara {
	struct PBR_Shader::Helpers {
		struct Settings {
			alignas(16) Vector3 albedo = Vector3(1.0f);
			alignas(4) float metalness = 0.1f;
			alignas(4) float roughness = 0.5f;
			alignas(8) Vector2 tiling = Vector2(1.0f);
			alignas(8) Vector2 offset = Vector2(0.0f);
		};

		static const constexpr std::string_view SETTINGS_NAME = "settings";
		static const constexpr std::string_view BASE_COLOR_NAME = "baseColor";
		static const constexpr std::string_view NORMAL_MAP_NAME = "normalMap";
		static const constexpr std::string_view METALNESS_MAP_NAME = "metalnessMap";
		static const constexpr std::string_view ROUGHNESS_MAP_NAME = "roughnessMap";
		static const constexpr std::string_view OCCLUSION_MAP_NAME = "occlusionMap";

		struct SettingsSerializer : public virtual Serialization::SerializerList::From<Settings> {
			inline SettingsSerializer() : Serialization::ItemSerializer("Settings", "Material settings") {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Settings* target)const final override {
				JIMARA_SERIALIZE_FIELDS(target, recordElement) {
					JIMARA_SERIALIZE_FIELD(target->albedo, "Albedo", "Main color of the material",
						Object::Instantiate<Serialization::ColorAttribute>());
					JIMARA_SERIALIZE_FIELD(target->metalness, "Metalness", "Tells, if the material is metallic or dielectric",
						Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
					JIMARA_SERIALIZE_FIELD(target->roughness, "Roughness", "Tells, how rough the material surface is",
						Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
					JIMARA_SERIALIZE_FIELD(target->tiling, "Tiling", "Texture UV tiling");
					JIMARA_SERIALIZE_FIELD(target->offset, "Offset", "Texture UV offset");
				};
			}
		};
	};

	PBR_Shader::PBR_Shader() : Graphics::ShaderClass("Jimara/Data/Materials/PBR/Jimara_PBR_Shader") {}

	PBR_Shader* PBR_Shader::Instance() {
		static PBR_Shader instance;
		return &instance;
	}

	Reference<const PBR_Shader::ConstantBufferBinding> PBR_Shader::DefaultConstantBufferBinding(
		const std::string_view& name, Graphics::GraphicsDevice* device)const {
		if (name == Helpers::SETTINGS_NAME) return SharedConstantBufferBinding(Helpers::Settings(), device);
		else return nullptr;
	}

	Reference<const PBR_Shader::TextureSamplerBinding> PBR_Shader::DefaultTextureSamplerBinding(
		const std::string_view& name, Graphics::GraphicsDevice* device)const {
		if (name == Helpers::NORMAL_MAP_NAME)
			return ShaderClass::SharedTextureSamplerBinding(glm::pow(Vector4(0.5f, 0.5f, 1.0f, 1.0f), Vector4(2.2f)), device);
		else return ShaderClass::DefaultTextureSamplerBinding(name, device);
	}

	void PBR_Shader::SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const {
		{
			static const ConstantBufferSerializer<Helpers::Settings> serializer(
				Helpers::SETTINGS_NAME, Object::Instantiate<Helpers::SettingsSerializer>());
			serializer.Serialize(reportField, bindings);
		}
		{
			static const TextureSamplerSerializer serializer(Helpers::BASE_COLOR_NAME, "Base Color", "Base albedo color");
			reportField(serializer.Serialize(bindings));
		}
		{
			static const TextureSamplerSerializer serializer(Helpers::NORMAL_MAP_NAME, "Normal map", 
				"Tangent space normal map ");
			reportField(serializer.Serialize(bindings));
		}
#define PBR_Shader_SerializeBindings_MRO_HINT(channel) \
	"(note that only " channel " channel will be used by the shader; you can compress memory by storing RGB as metalness, roughness and occlusion, respectively)"
		{
			static const TextureSamplerSerializer serializer(Helpers::METALNESS_MAP_NAME, "Metalness", 
				"Metalness map " PBR_Shader_SerializeBindings_MRO_HINT("R"));
			reportField(serializer.Serialize(bindings));
		}
		{
			static const TextureSamplerSerializer serializer(Helpers::ROUGHNESS_MAP_NAME, "Roughness", 
				"Roughness map "  PBR_Shader_SerializeBindings_MRO_HINT("G"));
			reportField(serializer.Serialize(bindings));
		}
		{
			static const TextureSamplerSerializer serializer(Helpers::OCCLUSION_MAP_NAME, "Occlusion", 
				"Occlusion map "  PBR_Shader_SerializeBindings_MRO_HINT("B"));
			reportField(serializer.Serialize(bindings));
		}
#undef PBR_Shader_SerializeBindings_MRO_HINT
	}
}
