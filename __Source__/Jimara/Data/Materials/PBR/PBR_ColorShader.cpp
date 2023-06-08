#include "PBR_ColorShader.h"
#include "../../Serialization/Helpers/SerializerMacros.h"
#include "../../Serialization/Attributes/ColorAttribute.h"
#include "../../Serialization/Attributes/SliderAttribute.h"


namespace Jimara {
	struct PBR_ColorShader::Helpers {
		struct Settings {
			alignas(16) Vector3 albedo = Vector3(1.0f);
			alignas(4) float metalness = 0.1f;
			alignas(4) float roughness = 0.5f;
		};

		static const constexpr std::string_view SETTINGS_NAME = "settings";

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
				};
			}
		};
	};

	PBR_ColorShader::PBR_ColorShader() : Graphics::ShaderClass("Jimara/Data/Materials/PBR/Jimara_PBR_ColorShader") {}

	PBR_ColorShader* PBR_ColorShader::Instance() {
		static PBR_ColorShader instance;
		return &instance;
	}

	Reference<const Graphics::ShaderClass::ConstantBufferBinding> PBR_ColorShader::DefaultConstantBufferBinding(
		const std::string_view& name, Graphics::GraphicsDevice* device)const {
		if (name == Helpers::SETTINGS_NAME) return SharedConstantBufferBinding(Helpers::Settings(), device);
		else return nullptr;
	}

	void PBR_ColorShader::SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const {
		static const ConstantBufferSerializer<Helpers::Settings> serializer(
			Helpers::SETTINGS_NAME, Object::Instantiate<Helpers::SettingsSerializer>());
		serializer.Serialize(reportField, bindings);
	}
}
