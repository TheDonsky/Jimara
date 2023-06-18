#include "HDRILight.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"


namespace Jimara {
	struct HDRILight::Helpers {
		struct Data {
			alignas(16) Vector3 color = Vector3(1.0f);
			alignas(4) uint32_t textureID = 0u;
			alignas(4) float numMipLevels = 1.0f;
			alignas(4) float mipBias = 0.0f;
			alignas(4) uint32_t sampleCount = 0u;
		};
		static_assert(sizeof(Data) == 32u);

		class HDRILightDescriptor
			: public virtual LightDescriptor
			, public virtual LightDescriptor::ViewportData
			, public virtual JobSystem::Job {
		public:
			HDRILight* m_owner;

		private:
			const Reference<const Graphics::ShaderClass::TextureSamplerBinding> m_whiteTexture;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_textureIndex;

			Data m_data;
			LightInfo m_info;

			inline void UpdateData() {
				if (m_owner == nullptr) return;
				m_data.color = m_owner->m_color * m_owner->m_intensity;
				Graphics::TextureSampler* const sampler = (m_owner->Texture() == nullptr)
					? m_whiteTexture->BoundObject() : m_owner->Texture();
				if (m_textureIndex == nullptr || m_textureIndex->BoundObject() != sampler)
					m_textureIndex = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(sampler);
				m_data.textureID = m_textureIndex->Index();
				m_data.numMipLevels = static_cast<float>(sampler->TargetView()->MipLevelCount());
				m_data.mipBias = m_owner->m_mipBias;
				m_data.sampleCount = m_owner->m_sampleCount;
			}

		public:
			inline HDRILightDescriptor(HDRILight* owner, uint32_t typeId)
				: m_owner(owner)
				, m_whiteTexture(Graphics::ShaderClass::SharedTextureSamplerBinding(Vector4(1.0f), owner->Context()->Graphics()->Device()))
				, m_info{} {
				UpdateData();
				m_info.typeId = typeId;
				m_info.data = &m_data;
				m_info.dataSize = sizeof(Data);
			}

			inline virtual ~HDRILightDescriptor() {}

			inline virtual Reference<const ViewportData> GetViewportData(const ViewportDescriptor*) final override { return this; }
			inline virtual LightInfo GetLightInfo()const override { return m_info; }
			inline virtual AABB GetLightBounds()const override {
				static const AABB BOUNDS = [] {
					static const float inf = std::numeric_limits<float>::infinity();
					AABB bounds = {};
					bounds.start = Vector3(-inf, -inf, -inf);
					bounds.end = Vector3(inf, inf, inf);
					return bounds;
				}();
				return BOUNDS;
			}

		protected:
			// JobSystem::Job:
			virtual void Execute()override {
				if (m_owner == nullptr) return;
				UpdateData();
			}
			virtual void CollectDependencies(Callback<Job*>)override {}
		};
	};
	

	HDRILight::HDRILight(Component* parent, const std::string_view& name)
		: Component(parent, name)
		, m_allLights(LightDescriptor::Set::GetInstance(parent->Context())) {}

	HDRILight::~HDRILight() {
		OnComponentDisabled();
	}

	void HDRILight::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Color, SetColor, "Color", "Base color of the emission", Object::Instantiate<Serialization::ColorAttribute>());
			JIMARA_SERIALIZE_FIELD_GET_SET(Intensity, SetIntensity, "Intensity", "Color multiplier");
			JIMARA_SERIALIZE_FIELD_GET_SET(Texture, SetTexture, "Texture", "Environment HDRI texture");
			if (Texture() != nullptr)
				JIMARA_SERIALIZE_FIELD(m_mipBias, "Mip Bias", "Texture mip Bias");
			JIMARA_SERIALIZE_FIELD(m_sampleCount, "Sample Count", "Number of directional samples per fragment");
		};
	}

	void HDRILight::OnComponentEnabled() {
		if (!ActiveInHeirarchy())
			OnComponentDisabled();
		else if (m_lightDescriptor == nullptr) {
			uint32_t typeId;
			if (Context()->Graphics()->Configuration().ShaderLoader()->GetLightTypeId("Jimara_HDRI_Light", typeId)) {
				Reference<Helpers::HDRILightDescriptor> descriptor = Object::Instantiate<Helpers::HDRILightDescriptor>(this, typeId);
				m_lightDescriptor = Object::Instantiate<LightDescriptor::Set::ItemOwner>(descriptor);
				m_allLights->Add(m_lightDescriptor);
				Context()->Graphics()->SynchPointJobs().Add(descriptor);
			}
		}
	}

	void HDRILight::OnComponentDisabled() {
		if (ActiveInHeirarchy())
			OnComponentEnabled();
		else {
			if (m_lightDescriptor != nullptr) {
				m_allLights->Remove(m_lightDescriptor);
				Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<JobSystem::Job*>(m_lightDescriptor->Item()));
				dynamic_cast<Helpers::HDRILightDescriptor*>(m_lightDescriptor->Item())->m_owner = nullptr;
				m_lightDescriptor = nullptr;
			}
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<HDRILight>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<HDRILight> serializer("Jimara/Lights/HDRILight", "HDR Texture component");
		report(&serializer);
	}
}
