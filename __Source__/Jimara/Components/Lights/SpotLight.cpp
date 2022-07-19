#include "SpotLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include "../../Data/Serialization/Attributes/SliderAttribute.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Environment/Rendering/LightingModels/DepthOnlyRenderer/DepthOnlyRenderer.h"


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

			// Shadow map parameters:
			alignas(4) float closePlane = 0.001f;	// Bytes [56 - 60)		Close plane of the projection matrix
			alignas(4) uint32_t shadowSamplerId;	// Bytes [60 - 64)		BindlessSamplers::GetFor(shadowTexture).Index();

			// Spotlight color and texture:
			alignas(16) Vector3 baseColor;			// Bytes [64 - 76)		Color() * Intensity();
			alignas(4) uint32_t colorSamplerId;		// Bytes [60 - 64)		BindlessSamplers::GetFor(Texture()).Index();
		};

		class SpotLightDescriptor 
			: public virtual LightDescriptor
			, public virtual JobSystem::Job
			, public virtual ViewportDescriptor {
		public:
			SpotLight* m_owner;

		private:
			const Reference<const Graphics::ShaderClass::TextureSamplerBinding> m_whiteTexture;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_texture;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_shadowTexture;

			Matrix4 m_poseMatrix = Math::Identity();
			float m_coneAngle = 30.0f;

			Data m_data;
			LightInfo m_info;

			inline void UpdateShadowRenderer() {
				if (m_owner->m_shadowResolution <= 0u) {
					if (m_owner->m_shadowRenderJob != nullptr) {
						m_owner->Context()->Graphics()->RenderJobs().Remove(m_owner->m_shadowRenderJob);
						m_owner->m_shadowRenderJob = nullptr;
					}
					m_owner->m_shadowTexture = nullptr;
				}
				else {
					Reference<DepthOnlyRenderer> renderer = m_owner->m_shadowRenderJob;

					if (m_owner->m_shadowRenderJob == nullptr) {
						renderer = Object::Instantiate<DepthOnlyRenderer>(this, LayerMask::All());
						m_owner->m_shadowRenderJob = renderer;
						if (m_owner->m_shadowTexture != nullptr)
							renderer->SetTargetTexture(m_owner->m_shadowTexture->TargetView());
						m_owner->Context()->Graphics()->RenderJobs().Add(m_owner->m_shadowRenderJob);
					}

					const Size3 textureSize = Size3(m_owner->m_shadowResolution, m_owner->m_shadowResolution, 1u);
					if (m_owner->m_shadowTexture == nullptr ||
						m_owner->m_shadowTexture->TargetView()->TargetTexture()->Size() != textureSize) {
						const auto texture = m_owner->Context()->Graphics()->Device()->CreateMultisampledTexture(
							Graphics::Texture::TextureType::TEXTURE_2D, renderer->TargetTextureFormat(), textureSize, 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
						const auto view = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
						m_owner->m_shadowTexture = view->CreateSampler();
						renderer->SetTargetTexture(m_owner->m_shadowTexture->TargetView());
					}
				}
			}

			inline void UpdateData() {
				// Transformation:
				{
					const Transform* transform = m_owner->GetTransfrom();
					if (transform == nullptr) {
						m_data.position = Vector3(0.0f);
						m_data.forward = Math::Forward();
						m_data.up = Math::Up();
						m_poseMatrix = Math::Identity();
					}
					else {
						m_data.position = transform->WorldPosition();
						Matrix4 poseMatrix = transform->WorldRotationMatrix();
						m_data.forward = poseMatrix[2];
						m_data.up = poseMatrix[1];
						poseMatrix[3] = Vector4(m_data.position, 1.0f);
						m_poseMatrix = Math::Inverse(poseMatrix);
					}
				}
				
				// Spotlight shape:
				{
					m_coneAngle = m_owner->OuterAngle();
					m_data.range = m_owner->Range();
					m_data.fadeAngleInvTangent = 1.0f / std::tan(Math::Radians(m_coneAngle));
					m_data.spotThreshold = std::tan(Math::Radians(m_owner->InnerAngle())) * m_data.fadeAngleInvTangent;
				}

				// Shadow map and 'projection color' sampler indices:
				{
					if (m_texture == nullptr || m_texture->BoundObject() != m_owner->Texture()) {
						if (m_owner->Texture() == nullptr) {
							if (m_texture == nullptr || m_texture->BoundObject() != m_whiteTexture->BoundObject())
								m_texture = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(m_whiteTexture->BoundObject());
						}
						else m_texture = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(m_owner->Texture());
					}
					m_data.colorSamplerId = m_texture->Index();

					if (m_shadowTexture == nullptr || m_shadowTexture->BoundObject() != m_owner->m_shadowTexture) {
						if (m_owner->m_shadowTexture == nullptr) {
							if (m_shadowTexture == nullptr || m_shadowTexture->BoundObject() != m_whiteTexture->BoundObject())
								m_shadowTexture = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(m_whiteTexture->BoundObject());
						}
						else m_shadowTexture = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(m_owner->m_shadowTexture);
					}
					m_data.shadowSamplerId = m_shadowTexture->Index();
				}

				// Spotlight color:
				m_data.baseColor = m_owner->Color() * m_owner->Intensity();
			}

		public:
			inline SpotLightDescriptor(SpotLight* owner, uint32_t typeId) 
				: ViewportDescriptor(owner->Context()), m_owner(owner)
				, m_whiteTexture(Graphics::ShaderClass::SharedTextureSamplerBinding(Vector4(1.0f), owner->Context()->Graphics()->Device()))
				, m_info {} {
				UpdateData();
				m_info.typeId = typeId;
				m_info.data = &m_data;
				m_info.dataSize = sizeof(Data);
			}


			// LightDescriptor:
			inline virtual LightInfo GetLightInfo()const override { return m_info; }
			inline virtual AABB GetLightBounds()const override {
				AABB bounds = {};
				bounds.start = m_data.position - Vector3(m_data.range);
				bounds.end = m_data.position + Vector3(m_data.range);
				return bounds;
			}


			// ViewportDescriptor:
			inline virtual Matrix4 ViewMatrix()const override { return m_poseMatrix; }
			inline virtual Matrix4 ProjectionMatrix(float aspect)const override {
				return Math::Perspective(m_coneAngle * 2.0f, aspect, m_data.closePlane, Math::Max(m_data.closePlane, m_data.range));
			}
			inline virtual Vector4 ClearColor()const override { return Vector4(0.0f, 0.0f, 0.0f, 1.0f); }


		protected:
			// JobSystem::Job:
			virtual void Execute()override {
				if (m_owner == nullptr) return;
				UpdateShadowRenderer();
				UpdateData(); 
			}
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
			JIMARA_SERIALIZE_FIELD_GET_SET(ShadowResolution, SetShadowResolution, "Shadow Resolution", "Resolution of the shadow",
				Object::Instantiate<Serialization::EnumAttribute<uint32_t>>(false,
					"No Shadows", 0u,
					"32 X 32", 32u,
					"64 X 64", 64u,
					"128 X 128", 128u,
					"256 X 256", 256u,
					"512 X 512", 512u,
					"1024 X 1024", 1024u,
					"2048 X 2048", 2048u));
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
		else {
			if (m_lightDescriptor != nullptr) {
				m_allLights->Remove(m_lightDescriptor);
				Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<JobSystem::Job*>(m_lightDescriptor->Item()));
				dynamic_cast<Helpers::SpotLightDescriptor*>(m_lightDescriptor->Item())->m_owner = nullptr;
				m_lightDescriptor = nullptr;
			}
			if (m_shadowRenderJob != nullptr) {
				Context()->Graphics()->RenderJobs().Remove(m_shadowRenderJob);
				m_shadowRenderJob = nullptr;
				m_shadowTexture = nullptr;
			}
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<SpotLight>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<SpotLight> serializer("Jimara/Lights/SpotLight", "Spot light component");
		report(&serializer);
	}
}
