#include "SpotLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include "../../Data/Serialization/Attributes/SliderAttribute.h"


namespace Jimara {
	struct SpotLight::Helpers {
		struct Data {
			// Transformation:
			alignas(16) Vector3 position;			// Bytes [0 - 12)		Transform::Position();
			alignas(16) Vector3 forward;			// Bytes [16 - 28)		Transform::Forward();
			alignas(16) Vector3 up;					// Bytes [32 - 44)		Transform::Up();

			// Spotlight shape:
			alignas(4) float range;					// Bytes [44 - 48)		Range();
			alignas(4) float spotThreshold;			// Bytes [48 - 52)		(tan(InnerAngle) / tan(OuterAngle));
			alignas(4) float fadeAngleInvTangent;	// Bytes [52 - 56)		(1.0 / tan(OuterAngle));

			// Shadow map and 'projection color' sampler indices:
			alignas(4) uint32_t shadowSamplerId;	// Bytes [56 - 60)		BindlessSamplers::GetFor(shadowTexture).Index();
			alignas(4) uint32_t colorSamplerId;		// Bytes [60 - 64)		BindlessSamplers::GetFor(Texture()).Index();

			// Spotlight color:
			alignas(16) Vector3 baseColor;			// Bytes [64 - 76)		Color() * Intensity();
													// Padd  [76 - 80)
		};

		class SpotLightDescriptor : public virtual LightDescriptor, public virtual JobSystem::Job {
		public:
			const SpotLight* m_owner;

		private:
			Reference<const Graphics::ShaderClass::TextureSamplerBinding> m_textureBinding;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_texture;
			
			Reference<const Graphics::ShaderClass::TextureSamplerBinding> m_shadowTextureBinding;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_shadowTexture;
			float m_shadowColor = 0.0f;
			
			Data m_data;
			LightInfo m_info;

			inline void UpdateData() {
				if (m_owner == nullptr) return;

				// Transformation:
				{
					const Transform* transform = m_owner->GetTransfrom();
					if (transform == nullptr) {
						m_data.position = Vector3(0.0f);
						m_data.forward = Math::Forward();
						m_data.up = Math::Up();
					}
					else {
						m_data.position = transform->WorldPosition();
						const Matrix4 rotation = transform->WorldRotationMatrix();
						m_data.forward = rotation[2];
						m_data.up = rotation[1];
					}
				}
				
				// Spotlight shape:
				{
					m_data.range = m_owner->Range();
					m_data.fadeAngleInvTangent = 1.0f / std::tan(Math::Radians(m_owner->OuterAngle()));
					m_data.spotThreshold = std::tan(Math::Radians(m_owner->InnerAngle())) * m_data.fadeAngleInvTangent;
				}

				// Shadow map and 'projection color' sampler indices:
				{
					if (m_texture == nullptr || m_texture->BoundObject() != m_owner->Texture()) {
						if (m_owner->Texture() == nullptr) {
							m_textureBinding = Graphics::ShaderClass::SharedTextureSamplerBinding(Vector4(1.0f), m_owner->Context()->Graphics()->Device());
							m_texture = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(m_textureBinding->BoundObject());
						}
						else {
							m_textureBinding = nullptr;
							m_texture = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(m_owner->Texture());
						}
					}
					m_data.colorSamplerId = m_texture->Index();

					if (m_shadowTexture == nullptr || m_shadowColor != m_owner->m_testShadowRange) {
						m_shadowColor = m_owner->m_testShadowRange;
						m_shadowTextureBinding = Graphics::ShaderClass::SharedTextureSamplerBinding(Vector4(m_shadowColor), m_owner->Context()->Graphics()->Device());
						m_shadowTexture = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(m_shadowTextureBinding->BoundObject());
					}
					m_data.shadowSamplerId = m_shadowTexture->Index();
				}

				// Spotlight color:
				m_data.baseColor = m_owner->Color() * m_owner->Intensity();
			}

		public:
			inline SpotLightDescriptor(const SpotLight* owner, uint32_t typeId) : m_owner(owner), m_info {} {
				UpdateData();
				m_info.typeId = typeId;
				m_info.data = &m_data;
				m_info.dataSize = sizeof(Data);
			}

			virtual LightInfo GetLightInfo()const override { return m_info; }

			virtual AABB GetLightBounds()const override {
				AABB bounds = {};
				bounds.start = m_data.position - Vector3(m_data.range);
				bounds.end = m_data.position + Vector3(m_data.range);
				return bounds;
			}

		protected:
			virtual void Execute()override { UpdateData(); }
			virtual void CollectDependencies(Callback<Job*>)override {}
		};
	};

	SpotLight::SpotLight(Component* parent, const std::string_view& name)
		: Component(parent, name)
		, m_allLights(LightDescriptor::Set::GetInstance(parent->Context())) {}

	SpotLight::~SpotLight() {
		OnComponentDisabled();
	}

	void SpotLight::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Range, SetRange, "Range", "Maximal distance, the SpotLight will illuminate at");
			JIMARA_SERIALIZE_FIELD_GET_SET(InnerAngle, SetInnerAngle, "Inner Angle", "Projection cone angle, before the intencity starts fading out",
				Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 90.0f));
			JIMARA_SERIALIZE_FIELD_GET_SET(OuterAngle, SetOuterAngle, "Outer Angle", "Projection cone angle at which the intencity will drop to zero",
				Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 90.0f));
			JIMARA_SERIALIZE_FIELD_GET_SET(Color, SetColor, "Color", "Base color of spotlight emission", Object::Instantiate<Serialization::ColorAttribute>());
			JIMARA_SERIALIZE_FIELD_GET_SET(Intensity, SetIntensity, "Intensity", "Color multiplier");
			JIMARA_SERIALIZE_FIELD_GET_SET(Texture, SetTexture, "Texture", "Optionally, the spotlight projection can take color form this texture");
			JIMARA_SERIALIZE_FIELD(m_testShadowRange, "ShadowRange", "Test thing for shadow map controls, before we actually render shadows",
				Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
		};
	}

	void SpotLight::OnComponentEnabled() {
		if (!ActiveInHeirarchy())
			OnComponentDisabled();
		else if (m_lightDescriptor == nullptr) {
			uint32_t typeId;
			if (Context()->Graphics()->Configuration().ShaderLoader()->GetLightTypeId("Jimara_SpotLight", typeId)) {
				Reference<Helpers::SpotLightDescriptor> descriptor = Object::Instantiate<Helpers::SpotLightDescriptor>(this, typeId);
				m_lightDescriptor = Object::Instantiate<LightDescriptor::Set::ItemOwner>(descriptor);
				m_allLights->Add(m_lightDescriptor);
				Context()->Graphics()->SynchPointJobs().Add(descriptor);
			}
		}
	}

	void SpotLight::OnComponentDisabled() {
		if (ActiveInHeirarchy())
			OnComponentEnabled();
		else if (m_lightDescriptor != nullptr) {
			m_allLights->Remove(m_lightDescriptor);
			Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<JobSystem::Job*>(m_lightDescriptor->Item()));
			dynamic_cast<Helpers::SpotLightDescriptor*>(m_lightDescriptor->Item())->m_owner = nullptr;
			m_lightDescriptor = nullptr;
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<SpotLight>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<SpotLight> serializer("Jimara/Lights/SpotLight", "Spot light component");
		report(&serializer);
	}
}
