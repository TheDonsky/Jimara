#include "PointLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include "../../Data/Serialization/Attributes/SliderAttribute.h"
#include "../../Environment/Rendering/LightingModels/DepthOnlyRenderer/DualParaboloidDepthRenderer.h"


namespace Jimara {
	struct PointLight::Helpers {
		class PointLightDescriptor : public virtual LightDescriptor, public virtual JobSystem::Job {
		public:
			PointLight* m_owner;

		private:
			struct Data {
				alignas(16) Vector3 position;
				alignas(16) Vector3 color;
			} m_data;

			float m_radius;

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
					Reference<DualParaboloidDepthRenderer> shadowMapper = m_owner->m_shadowRenderJob;
					if (shadowMapper == nullptr) {
						shadowMapper = Object::Instantiate<DualParaboloidDepthRenderer>(m_owner->Context(), LayerMask::All());
						m_owner->m_shadowRenderJob = shadowMapper;
						m_owner->m_shadowTexture = nullptr;
						m_owner->Context()->Graphics()->RenderJobs().Add(m_owner->m_shadowRenderJob);
					}

					const Size3 textureSize = Size3(m_owner->m_shadowResolution, m_owner->m_shadowResolution, 1u);
					if (m_owner->m_shadowTexture == nullptr ||
						m_owner->m_shadowTexture->TargetView()->TargetTexture()->Size() != textureSize) {
						const auto texture = m_owner->Context()->Graphics()->Device()->CreateMultisampledTexture(
							Graphics::Texture::TextureType::TEXTURE_2D, shadowMapper->TargetTextureFormat(),
							textureSize, 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
						const auto view = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
						const auto sampler = view->CreateSampler();
						shadowMapper->SetTargetTexture(view);
						m_owner->m_shadowTexture = sampler;
					}
				}
			}

			void UpdateData() {
				if (m_owner == nullptr) return;
				const Transform* transform = m_owner->GetTransfrom();
				if (transform == nullptr) m_data.position = Vector3(0.0f, 0.0f, 0.0f);
				else m_data.position = transform->WorldPosition();
				m_data.color = m_owner->Color();
				m_radius = m_owner->Radius();
			}

		public:
			inline PointLightDescriptor(PointLight* owner, uint32_t typeId) : m_owner(owner), m_info{} {
				UpdateData();
				m_info.typeId = typeId;
				m_info.data = &m_data;
				m_info.dataSize = sizeof(Data);
			}

			virtual LightInfo GetLightInfo()const override { return m_info; }

			virtual AABB GetLightBounds()const override {
				AABB bounds = {};
				bounds.start = m_data.position - Vector3(m_radius, m_radius, m_radius);
				bounds.end = m_data.position + Vector3(m_radius, m_radius, m_radius);
				return bounds;
			}

		protected:
			virtual void Execute()override { 
				UpdateShadowRenderer();
				UpdateData(); 
				Reference<DualParaboloidDepthRenderer> shadowMapper = m_owner->m_shadowRenderJob;
				if (shadowMapper != nullptr)
					shadowMapper->Configure(m_data.position, 0.001f, m_owner->Radius());
			}
			virtual void CollectDependencies(Callback<Job*>)override {}
		};
	};

	PointLight::PointLight(Component* parent, const std::string_view& name, Vector3 color, float radius)
		: Component(parent, name)
		, m_allLights(LightDescriptor::Set::GetInstance(parent->Context()))
		, m_color(color)
		, m_radius(radius) {}

	PointLight::~PointLight() {
		OnComponentDisabled();
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<PointLight>(const Callback<const Object*>& report) { 
		static const ComponentSerializer::Of<PointLight> serializer("Jimara/Lights/PointLight", "Point light component");
		report(&serializer);
	}


	Vector3 PointLight::Color()const { return m_color; }

	void PointLight::SetColor(Vector3 color) { m_color = color; }


	float PointLight::Radius()const { return m_radius; }

	void PointLight::SetRadius(float radius) { m_radius = radius <= 0.0f ? 0.0f : radius; }

	void PointLight::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Color, SetColor, "Color", "Light Color", Object::Instantiate<Serialization::ColorAttribute>());
			JIMARA_SERIALIZE_FIELD_GET_SET(Radius, SetRadius, "Radius", "Maximal illuminated distance");
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
			if (ShadowResolution() > 0u) {
				JIMARA_SERIALIZE_FIELD_GET_SET(ShadowSoftness, SetShadowSoftness, "Shadow Softness", "Tells, how soft the cast shadow is",
					Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
				JIMARA_SERIALIZE_FIELD_GET_SET(ShadowFilterSize, SetShadowFilterSize, "Filter Size", "Tells, what size kernel is used for rendering soft shadows",
					Object::Instantiate<Serialization::SliderAttribute<uint32_t>>(1u, 65u, 2u));
			}
		};
	}

	void PointLight::OnComponentEnabled() {
		if (!ActiveInHeirarchy())
			OnComponentDisabled();
		else if (m_lightDescriptor == nullptr) {
			uint32_t typeId;
			if (Context()->Graphics()->Configuration().ShaderLoader()->GetLightTypeId("Jimara_PointLight", typeId)) {
				Reference<Helpers::PointLightDescriptor> descriptor = Object::Instantiate<Helpers::PointLightDescriptor>(this, typeId);
				m_lightDescriptor = Object::Instantiate<LightDescriptor::Set::ItemOwner>(descriptor);
				m_allLights->Add(m_lightDescriptor);
				Context()->Graphics()->SynchPointJobs().Add(descriptor);
			}
		}
	}

	void PointLight::OnComponentDisabled() {
		if (ActiveInHeirarchy())
			OnComponentEnabled();
		else {
			if (m_lightDescriptor != nullptr) {
				m_allLights->Remove(m_lightDescriptor);
				Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<JobSystem::Job*>(m_lightDescriptor->Item()));
				dynamic_cast<Helpers::PointLightDescriptor*>(m_lightDescriptor->Item())->m_owner = nullptr;
				m_lightDescriptor = nullptr;
			}
			if (m_shadowRenderJob != nullptr) {
				Context()->Graphics()->RenderJobs().Remove(m_shadowRenderJob);
				m_shadowRenderJob = nullptr;
				m_shadowTexture = nullptr;
			}
		}
	}
}
