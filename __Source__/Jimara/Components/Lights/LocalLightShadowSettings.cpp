#include "LocalLightShadowSettings.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Data/Serialization/Attributes/SliderAttribute.h"


namespace Jimara {
	void LocalLightShadowSettings::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(ShadowResolution, SetShadowResolution, "Shadow Resolution", "Resolution of the shadow",
				Object::Instantiate<Serialization::EnumAttribute<uint32_t>>(false,
					"No Shadows", 0u,
					"32", 32u,
					"64", 64u,
					"128", 128u,
					"256", 256u,
					"512", 512u,
					"1024", 1024u,
					"2048", 2048u));
			if (ShadowResolution() > 0u) {
				JIMARA_SERIALIZE_FIELD_GET_SET(ShadowSoftness, SetShadowSoftness, "Shadow Softness", "Tells, how soft the cast shadow is",
					Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
				JIMARA_SERIALIZE_FIELD_GET_SET(ShadowFilterSize, SetShadowFilterSize, "Filter Size", "Tells, what size kernel is used for rendering soft shadows",
					Object::Instantiate<Serialization::SliderAttribute<uint32_t>>(1u, 65u, 2u));
				JIMARA_SERIALIZE_FIELD_GET_SET(ShadowDistance, SetShadowDistance, "Shadow Distance",
					"Shadow distance from viewport origin, before it starts fading");
				JIMARA_SERIALIZE_FIELD_GET_SET(ShadowFadeDistance, SetShadowFadeDistance, "Shadow Fade Distance",
					"Shadow fade-out distance after ShadowDistance, before it fully disapears");
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<LocalLightShadowSettings>(const Callback<const Object*>& report) {
		static const Reference<ConfigurableResource::ResourceFactory> factory = ConfigurableResource::ResourceFactory::Create<LocalLightShadowSettings>(
			"Local Light Shadow Settings", "Jimara/Lights/Local Light Shadow Settings", "Shadowmapper settings for local light sources like spot and/or point");
		report(factory);
	}
}
