#include "HDRILight.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include "../../Environment/Rendering/ImageBasedLighting/HDRISkyboxRenderer.h"


namespace Jimara {
	struct HDRILight::Helpers {
		struct Data {
			alignas(16) Vector3 color = Vector3(1.0f);
			alignas(4) uint32_t irradianceID = 0u;
			alignas(4) uint32_t preFilteredMapID = 0u;
			alignas(4) uint32_t environmentMapID = 0u;
			alignas(4) uint32_t brdfIntegrationMapID = 0u;
			alignas(4) float preFilteredMapMipCount = 1.0f;
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
			const Reference<const Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_brdfIntegrationMapIndex;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_irradianceIndex;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_preFilteredMapIndex;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_environmentMapIndex;

			Data m_data;
			LightInfo m_info;

			inline void UpdateData() {
				if (m_owner == nullptr) return;
				m_data.color = m_owner->m_color * m_owner->m_intensity;
				auto updateTextureId = [&](auto& binding, auto* sampler, auto& index) {
					if (binding == nullptr || binding->BoundObject() != sampler)
						binding = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(
							sampler != nullptr ? sampler : m_whiteTexture->BoundObject());
					index = binding->Index();
				};
				updateTextureId(m_irradianceIndex, m_owner->Texture() == nullptr ? nullptr : m_owner->Texture()->IrradianceMap(), m_data.irradianceID);
				updateTextureId(m_preFilteredMapIndex, m_owner->Texture() == nullptr ? nullptr : m_owner->Texture()->PreFilteredMap(), m_data.preFilteredMapID);
				updateTextureId(m_environmentMapIndex, m_owner->Texture() == nullptr ? nullptr : m_owner->Texture()->HDRI(), m_data.environmentMapID);
				m_data.brdfIntegrationMapID = (m_brdfIntegrationMapIndex == nullptr) ? 0u : m_brdfIntegrationMapIndex->Index();
				m_data.preFilteredMapMipCount = float(m_preFilteredMapIndex->BoundObject()->TargetView()->TargetTexture()->MipLevels());
			}


			Reference<RenderStack> m_renderStack;
			Reference<const ViewportDescriptor> m_skyboxViewport;
			Reference<HDRISkyboxRenderer> m_skyboxRenderer;
			
			inline void RecreateSkyboxRenderer() {
				const Reference<const ViewportDescriptor> viewport =
					(m_owner == nullptr || m_owner->Camera() == nullptr) ? nullptr
					: m_owner->Camera()->ViewportDescriptor();
				if (viewport == m_skyboxViewport)
					return;
				if (m_skyboxRenderer != nullptr)
					m_renderStack->RemoveRenderer(m_skyboxRenderer);
				m_skyboxViewport = nullptr;
				m_skyboxRenderer = nullptr;
				if (viewport == nullptr)
					return;
				if (m_renderStack == nullptr) {
					m_renderStack = RenderStack::Main(m_owner->Context());
					if (m_renderStack == nullptr) {
						m_owner->Context()->Log()->Error(
							"HDRILight::Helpers::HDRILightDescriptor - Failed to get render stack for rendering skybox! ", 
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return;
					}
				}
				m_skyboxRenderer = HDRISkyboxRenderer::Create(viewport);
				if (m_skyboxRenderer == nullptr) {
					m_owner->Context()->Log()->Error(
						"HDRILight::Helpers::HDRILightDescriptor - Failed to create skybox renderer! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				m_renderStack->AddRenderer(m_skyboxRenderer);
				m_skyboxViewport = viewport;
			}

			inline void UpdateSkyboxRenderer() {
				if (m_skyboxRenderer == nullptr)
					return;
				m_skyboxRenderer->SetCategory(m_owner->Camera()->RendererCategory());
				m_skyboxRenderer->SetPriority(m_owner->Camera()->RendererPriority() + 1u);
				m_skyboxRenderer->SetEnvironmentMap(m_owner->Texture() == nullptr ? nullptr : m_owner->Texture()->HDRI());
				m_skyboxRenderer->SetColorMultiplier(Vector4(m_owner->Color() * m_owner->Intensity(), 1.0f));
			}


		public:
			inline HDRILightDescriptor(HDRILight* owner, uint32_t typeId)
				: m_owner(owner)
				, m_whiteTexture(Graphics::ShaderClass::SharedTextureSamplerBinding(Vector4(1.0f), owner->Context()->Graphics()->Device()))
				, m_brdfIntegrationMapIndex([&]() {
				const Reference<Graphics::TextureSampler> brdfIntegrationMap = HDRIEnvironment::BrdfIntegrationMap(
					owner->Context()->Graphics()->Device(), owner->Context()->Graphics()->Configuration().ShaderLoader());
				return owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(brdfIntegrationMap);
					}())
				, m_info{} {
				UpdateData();
				m_info.typeId = typeId;
				m_info.data = &m_data;
				m_info.dataSize = sizeof(Data);
			}

			inline virtual ~HDRILightDescriptor() {
				Dispose();
			}

			inline void Dispose() {
				m_owner = nullptr;
				RecreateSkyboxRenderer();
			}

			inline virtual Reference<const LightDescriptor::ViewportData> GetViewportData(const ViewportDescriptor*) final override { return this; }
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
				RecreateSkyboxRenderer();
				UpdateSkyboxRenderer();
			}
			virtual void CollectDependencies(Callback<Job*>)override {}
		};
	};
	

	HDRILight::HDRILight(Component* parent, const std::string_view& name)
		: Component(parent, name)
		, m_allLights(LightDescriptor::Set::GetInstance(parent->Context())) {}

	HDRILight::~HDRILight() {
		SetCamera(nullptr);
		OnComponentDisabled();
	}

	void HDRILight::SetCamera(Jimara::Camera* camera) {
		if (m_camera == camera)
			return;

		typedef void(*CameraDestroyedCallback)(HDRILight*, Component*);
		static const CameraDestroyedCallback onCameraDestroyed = [](HDRILight* self, Component*) {
			self->SetCamera(nullptr);
		};

		if (m_camera != nullptr)
			m_camera->OnDestroyed() -= Callback(onCameraDestroyed, this);
		m_camera = camera;
		if (m_camera != nullptr)
			m_camera->OnDestroyed() += Callback(onCameraDestroyed, this);
	}

	void HDRILight::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Color, SetColor, "Color", "Base color of the emission", Object::Instantiate<Serialization::ColorAttribute>());
			JIMARA_SERIALIZE_FIELD_GET_SET(Intensity, SetIntensity, "Intensity", "Color multiplier");
			JIMARA_SERIALIZE_FIELD_GET_SET(Texture, SetTexture, "Texture", "Environment HDRI texture");
			JIMARA_SERIALIZE_FIELD_GET_SET(Camera, SetCamera, "Camera", "If set, skybox will be rendered before the camera renders scene");
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
				dynamic_cast<Helpers::HDRILightDescriptor*>(m_lightDescriptor->Item())->Dispose();
				m_lightDescriptor = nullptr;
			}
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<HDRILight>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<HDRILight> serializer("Jimara/Lights/HDRILight", "HDR Texture component");
		report(&serializer);
	}
}
