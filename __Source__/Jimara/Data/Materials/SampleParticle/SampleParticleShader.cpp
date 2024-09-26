#include "SampleParticleShader.h"
#include "../../Serialization/Helpers/SerializerMacros.h"
#include "../../Serialization/Attributes/ColorAttribute.h"
#include "../../Serialization/Attributes/SliderAttribute.h"


namespace Jimara {
	struct SampleParticleShader::Helpers {
		struct MaterialSettings {
			alignas(16) Vector3 baseColor = Vector3(1.0f);
			alignas(4) float alphaThreshold = -1.0f;
		};

		struct SettingsSerializer : public virtual Serialization::SerializerList::From<MaterialSettings> {
			inline SettingsSerializer() : Serialization::ItemSerializer("SampleParticleShader::Settings", "Settings serializer for SampleParticleShader") {}
			inline virtual ~SettingsSerializer() {}
			inline virtual void GetFields(const Callback<Serialization::SerializedObject>&, MaterialSettings*)const override {}
		};

		static const constexpr std::string_view SettingsBufferName() { return "materialSettings"; }
		static const constexpr std::string_view TexSamplerName() { return "texSampler"; }
	};

	SampleParticleShader* SampleParticleShader::Instance() {
		static SampleParticleShader instance;
		return &instance;
	}

	Reference<const Graphics::ShaderClass::ConstantBufferBinding> SampleParticleShader::DefaultConstantBufferBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const {
		if (name == Helpers::SettingsBufferName()) 
			return Graphics::SharedConstantBufferBinding<Helpers::MaterialSettings>({}, device);
		else return nullptr;
	}

	void SampleParticleShader::SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const {
		static const Helpers::SettingsSerializer settingsSerializer;
		static const ConstantBufferSerializer<Helpers::MaterialSettings> bufferSerializer(Helpers::SettingsBufferName(), &settingsSerializer, {});
		Helpers::MaterialSettings settings = {};

		// Extract settings:
		{
			auto serializeData = [&](const Serialization::SerializedObject& serializedObject) {
				settings = *reinterpret_cast<Helpers::MaterialSettings*>(serializedObject.TargetAddr());
			};
			bufferSerializer.Serialize(Callback<Serialization::SerializedObject>::FromCall(&serializeData), bindings);
		}

		// Serialize fields:
		JIMARA_SERIALIZE_FIELDS(bindings, reportField) {
			JIMARA_SERIALIZE_FIELD(settings.baseColor, "Base Color", "Diffuse Color", Object::Instantiate<Serialization::ColorAttribute>());
			{
				static const TextureSamplerSerializer serializer(Helpers::TexSamplerName(), "Diffuse", "Diffuse texture");
				reportField(serializer.Serialize(bindings));
			}
			if (bindings->GetTextureSampler(Helpers::TexSamplerName()) != nullptr) {
				settings.alphaThreshold = Math::Min(Math::Max(settings.alphaThreshold, 0.0f), 1.0f);
				JIMARA_SERIALIZE_FIELD(settings.alphaThreshold, "Alphas Threshold", "Minimal alpha not to discard",
					Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
				settings.alphaThreshold = Math::Min(Math::Max(settings.alphaThreshold, 0.0f), 1.0f);
			}
			else settings.alphaThreshold = -1.0f;
		};

		// Store settings:
		{
			auto serializeData = [&](const Serialization::SerializedObject& serializedObject) {
				(*reinterpret_cast<Helpers::MaterialSettings*>(serializedObject.TargetAddr())) = settings;
			};
			bufferSerializer.Serialize(Callback<Serialization::SerializedObject>::FromCall(&serializeData), bindings);
		}
	}

	SampleParticleShader::SampleParticleShader() : Graphics::ShaderClass(
		"Jimara/Data/Materials/SampleParticle/Jimara_SampleParticleShader", Graphics::GraphicsPipeline::BlendMode::ALPHA_BLEND) {}
}
