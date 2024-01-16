#include "SpotLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include "../../Data/Serialization/Attributes/SliderAttribute.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Environment/Rendering/LightingModels/DepthOnlyRenderer/DepthOnlyRenderer.h"
#include "../../Environment/Rendering/Shadows/VarianceShadowMapper/VarianceShadowMapper.h"
#include "../../Environment/Rendering/TransientImage.h"
#include "../../Core/Stopwatch.h"


namespace Jimara {
	struct SpotLight::Helpers {
		struct Viewport : public virtual ViewportDescriptor {
			Matrix4 viewMatrix = Math::Identity();
			Matrix4 projectionMatrix = Math::Identity();
			const Reference<const RendererFrustrumDescriptor> mainViewport;

			inline Viewport(SceneContext* context, const RendererFrustrumDescriptor* mainView) 
				: ViewportDescriptor(context), mainViewport(mainView) {}
			inline virtual Matrix4 ViewMatrix()const override { return viewMatrix; }
			inline virtual Matrix4 ProjectionMatrix()const override { return projectionMatrix; }
			inline virtual Vector4 ClearColor()const override { return Vector4(0.0f); }
			inline virtual const RendererFrustrumDescriptor* ViewportFrustrumDescriptor()const override { return mainViewport; }
		};

		struct ShadowMapper : public virtual LightmapperJobs::Job {
			const Reference<Viewport> view;
			const Reference<SceneContext> context;
			const Reference<DepthOnlyRenderer> depthRenderer;
			const Reference<VarianceShadowMapper> shadowMapper;
			float timeLeft = 0.0f;

			inline ShadowMapper(ViewportDescriptor* viewport, const RendererFrustrumDescriptor* graphicsObjectFrustrum)
				: view(dynamic_cast<Viewport*>(viewport)), context(viewport->Context())
				, depthRenderer(Object::Instantiate<DepthOnlyRenderer>(viewport, LayerMask::All(), graphicsObjectFrustrum))
				, shadowMapper(VarianceShadowMapper::Create(viewport->Context())) {}

			inline virtual ~ShadowMapper() {}

			inline virtual void Execute() override {
				const Graphics::InFlightBufferInfo commandBufferInfo = context->Graphics()->GetWorkerThreadCommandBuffer();
				depthRenderer->Render(commandBufferInfo);
				shadowMapper->GenerateVarianceMap(commandBufferInfo);
			}
			inline virtual void CollectDependencies(Callback<JobSystem::Job*> record) override { 
				depthRenderer->GetDependencies(record); 
			}
		};

		struct Data {
			// Transformation & shape:
			alignas(16) Vector3 position = Vector3(0.0f);	// Bytes [0 - 12)		Transform::Position();
			alignas(4) float range = 10.0f;					// Bytes [12 - 16)		Range();
			alignas(16) Vector3 forward = Math::Forward();	// Bytes [16 - 28)		Transform::Forward();
			alignas(4) float spotThreshold = 0.01f;			// Bytes [28 - 32)		(tan(InnerAngle) / tan(OuterAngle));
			alignas(16) Vector3 up = Math::Up();			// Bytes [32 - 44)		Transform::Up();
			alignas(4) float fadeAngleInvTangent = 0.0f;	// Bytes [44 - 48)		(1.0 / tan(OuterAngle));

			// Shadow map parameters:
			alignas(4) float shadowStrength = 1.0f;			// Bytes [48 - 52)		Multiplier for shadowmap strength
			alignas(4) float depthEpsilon = 0.001f;			// Bytes [52 - 56)		Error margin for elleminating shimmering caused by floating point inaccuracies from the depth map.
			alignas(4) uint32_t shadowSamplerId = 0u;		// Bytes [56 - 60)		BindlessSamplers::GetFor(shadowTexture).Index();
			alignas(4) uint32_t pad_0 = 0u;					// Bytes [60 - 64)

			// Spotlight color and texture:
			alignas(8) Vector2 colorTiling = Vector2(1.0f);	// Bytes [64 - 72)		TextureTiling();
			alignas(8) Vector2 colorOffset = Vector2(0.0f);	// Bytes [72 - 80)		TextureOffset();
			alignas(16) Vector3 baseColor = Vector3(1.0f);	// Bytes [80 - 92)		Color() * Intensity();
			alignas(4) uint32_t colorSamplerId = 0u;		// Bytes [92 - 96)		BindlessSamplers::GetFor(Texture()).Index();
		};

		static_assert(sizeof(Data) == 96);
		static_assert(offsetof(Data, colorTiling) == 64);

		struct ShadowSettings {
			Matrix4 poseMatrix = Math::Identity();
			uint32_t shadowResolution = 0u;
			float shadowDistance = 0.0f;
			float shadowFadeDistance = 0.0f;
			float shadowStrengthMultiplier = 1.0f;
			float coneAngle = 0.0f;
			float softness = 0.0f;
			uint32_t filterSize = 1u;
		};

		template<typename... Args>
		class EventObject : public virtual Object {
		private:
			mutable EventInstance<Args...> m_event;

		public:
			inline virtual ~EventObject() {}
			inline Event<Args...>& OnTick()const { return m_event; }
			inline void Tick(const Args&... args) { m_event(args...); }
		};

#pragma warning(disable: 4250)
		class ViewportShadowmapperCache : public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<const RendererFrustrumDescriptor> m_frustrum;
			const Reference<const EventObject<>> m_cacheTick;

			std::mutex m_shadowmapperLock;
			std::vector<Reference<ShadowMapper>> m_shadowmappers;
			Stopwatch m_shadowmapperStopwatch;

			static const constexpr float SHADOWMAPPER_DISCARD_TIMEOUT = 8.0f;

			void OnTick() {
				if (m_shadowmapperStopwatch.Elapsed() < 0.01f)
					return;
				std::unique_lock<std::mutex> lock(m_shadowmapperLock);
				const float deltaTime = m_shadowmapperStopwatch.Reset();
				size_t liveCount = 0u;
				for (size_t i = 0u; i < m_shadowmappers.size(); i++) {
					Reference<ShadowMapper> shadowmapper = m_shadowmappers[i];
					shadowmapper->timeLeft -= deltaTime;
					if (shadowmapper->timeLeft > 0.0f) {
						m_shadowmappers[liveCount] = shadowmapper;
						liveCount++;
					}
				}
				m_shadowmappers.resize(liveCount);
			}

		public:
			inline ViewportShadowmapperCache(SceneContext* context, const RendererFrustrumDescriptor* frustrum, const EventObject<>* tick)
				: m_context(context), m_frustrum(frustrum), m_cacheTick(tick) {
				assert(m_context != nullptr);
				assert(m_cacheTick != nullptr);
				m_cacheTick->OnTick() += Callback(&ViewportShadowmapperCache::OnTick, this);
			}
			inline virtual ~ViewportShadowmapperCache() {
				m_cacheTick->OnTick() -= Callback(&ViewportShadowmapperCache::OnTick, this);
			}

			Reference<ShadowMapper> GetShadowmapper() {
				{
					std::unique_lock<std::mutex> lock(m_shadowmapperLock);
					if (!m_shadowmappers.empty()) {
						Reference<ShadowMapper> rv = m_shadowmappers.back();
						m_shadowmappers.pop_back();
						return rv;
					}
				}
				Reference<Viewport> viewport = Object::Instantiate<Viewport>(m_context, m_frustrum);
				return Object::Instantiate<ShadowMapper>(viewport, viewport);
			}

			void ReleaseShadowmapper(ShadowMapper* shadowmapper) {
				if (shadowmapper == nullptr)
					return;
				std::unique_lock<std::mutex> lock(m_shadowmapperLock);
				m_shadowmappers.push_back(shadowmapper);
				shadowmapper->timeLeft = SHADOWMAPPER_DISCARD_TIMEOUT;
			}
		};

		class ShadowmapperCache
			: public virtual JobSystem::Job
			, public virtual ObjectCache<Reference<const Object>>
			, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<EventObject<>> m_tick = Object::Instantiate<EventObject<>>();

		public:
			inline ShadowmapperCache(SceneContext* context) : m_context(context) {}
			inline virtual ~ShadowmapperCache() {}

			static Reference<ShadowmapperCache> Get(SceneContext* context) {
				assert(context != nullptr);
				struct Cache : public virtual ObjectCache<Reference<const Object>> {
					std::mutex createLock;

					Reference<ShadowmapperCache> Get(SceneContext* ctx) {
						std::unique_lock<std::mutex> lock(createLock);
						return GetCachedOrCreate(ctx, [&]() {
							Reference<ShadowmapperCache> result = Object::Instantiate<ShadowmapperCache>(ctx);
							ctx->Graphics()->SynchPointJobs().Add(result);
							return result;
							});
					}
				};

				static Cache cache;
				return cache.Get(context);
			}

			inline Reference<ViewportShadowmapperCache> GetViewportCache(const RendererFrustrumDescriptor* frustrum) {
				return GetCachedOrCreate(frustrum, [&]() { return Object::Instantiate<ViewportShadowmapperCache>(m_context, frustrum, m_tick); });
			}

		protected:
			inline virtual void Execute() override { m_tick->Tick(); }
			virtual void CollectDependencies(Callback<Job*>) {}
		};


		class SpotLightData
			: public virtual LightDescriptor::ViewportData
			, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<const RendererFrustrumDescriptor> m_frustrum;
			const Reference<const EventObject<const Data&, const ShadowSettings&, LightDescriptor::Set*>> m_onUpdate;

			Data m_data = {};
			LightDescriptor::LightInfo m_info = {};

			Reference<ViewportShadowmapperCache> m_viewportShadowmappers;
			Reference<ShadowMapper> m_shadowmapper;

			Reference<LightmapperJobs> m_lightmapperJobs;
			Reference<LightmapperJobs::ItemOwner> m_shadowRenderJob;
			Reference<Graphics::TextureSampler> m_shadowTexture;
			Reference<const TransientImage> m_depthTexture;

			const Reference<const Graphics::ShaderClass::TextureSamplerBinding> m_noShadowTexture;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_shadowSamplerId;

			void Update(const Data& curData, const ShadowSettings& shadowSettings, LightDescriptor::Set* allLights) {
				// Figure out if we need a shadowmapper or not at all:
				float shadowFade;
				if (allLights == nullptr)
					shadowFade = (shadowSettings.shadowResolution > 0u) ? 1.0f : 0.0f;
				else if (shadowSettings.shadowResolution > 0u) {
					if (m_frustrum == nullptr)
						shadowFade = 0.0f;
					else {
						const Vector3 eyePos = m_frustrum->EyePosition();
						const float distance = Math::Magnitude(curData.position - eyePos);
						const float distanceFromEdge = distance - shadowSettings.shadowDistance;
						if (distanceFromEdge <= 0.0f)
							shadowFade = 1.0f;
						else if (distanceFromEdge < shadowSettings.shadowFadeDistance)
							shadowFade = 1.0f - (distanceFromEdge / shadowSettings.shadowFadeDistance);
						else shadowFade = 0.0f;
					}
				}
				else shadowFade = 0.0f;

				// Discard or aquire a shadowmapper:
				if (shadowFade > 0.0f && allLights != nullptr) {
					if (m_shadowmapper == nullptr) {
						if (m_viewportShadowmappers == nullptr)
							m_viewportShadowmappers = ShadowmapperCache::Get(m_context)->GetViewportCache(m_frustrum);
						if (m_viewportShadowmappers != nullptr)
							m_shadowmapper = m_viewportShadowmappers->GetShadowmapper();
						if (m_shadowmapper != nullptr) {
							// Activate shadowmapper:
							if (m_lightmapperJobs == nullptr)
								m_lightmapperJobs = LightmapperJobs::GetInstance(allLights);
							assert(m_lightmapperJobs != nullptr);
							m_shadowRenderJob = Object::Instantiate<LightmapperJobs::ItemOwner>(m_shadowmapper);
							m_context->Graphics()->RenderJobs().Add(m_shadowRenderJob->Item());
							m_lightmapperJobs->Add(m_shadowRenderJob);
						}
					}
				}
				else if (m_shadowmapper != nullptr) {
					// Deactivate shadowmapper:
					m_context->Graphics()->RenderJobs().Remove(m_shadowRenderJob->Item());
					m_lightmapperJobs->Remove(m_shadowRenderJob);
					m_shadowRenderJob = nullptr;
					m_viewportShadowmappers->ReleaseShadowmapper(m_shadowmapper);
					m_shadowmapper = nullptr;
				}

				// Update shadow texture:
				if (m_shadowmapper != nullptr) {
					// Update shadowmapper textures:
					const Size3 textureSize = Size3(shadowSettings.shadowResolution, shadowSettings.shadowResolution, 1u);
					if (m_shadowTexture == nullptr ||
						m_shadowTexture->TargetView()->TargetTexture()->Size() != textureSize) {
						m_depthTexture = TransientImage::Get(m_context->Graphics()->Device(),
							Graphics::Texture::TextureType::TEXTURE_2D, m_shadowmapper->depthRenderer->TargetTextureFormat(),
							textureSize, 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
						const auto view = m_depthTexture->Texture()->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
						const auto sampler = view->CreateSampler(
							Graphics::TextureSampler::FilteringMode::LINEAR,
							Graphics::TextureSampler::WrappingMode::REPEAT);
						m_shadowmapper->depthRenderer->SetTargetTexture(view);
						m_shadowTexture = m_shadowmapper->shadowMapper->SetDepthTexture(sampler, true);
					}

					// Update shadowmapper settings:
					const float closePlane = curData.range * curData.depthEpsilon;
					m_shadowmapper->view->viewMatrix = shadowSettings.poseMatrix;
					m_shadowmapper->view->projectionMatrix = 
						Math::Perspective(shadowSettings.coneAngle * 2.0f, 1.0f, closePlane, Math::Max(closePlane, curData.range));
					m_shadowmapper->shadowMapper->Configure(closePlane, curData.range, shadowSettings.softness, shadowSettings.filterSize);
				}
				else {
					// Remove textures:
					m_depthTexture = nullptr;
					m_shadowTexture = nullptr;
				}

				// Update data:
				{
					Graphics::TextureSampler* const shadowSampler =
						(m_shadowTexture == nullptr) ? m_noShadowTexture->BoundObject() : m_shadowTexture.operator->();
					assert(shadowSampler != nullptr);
					if (m_shadowSamplerId == nullptr || m_shadowSamplerId->BoundObject() != shadowSampler)
						m_shadowSamplerId = m_context->Graphics()->Bindless().Samplers()->GetBinding(shadowSampler);
					assert(m_shadowSamplerId != nullptr);

					m_data = curData;
					m_data.shadowSamplerId = m_shadowSamplerId->Index();
					m_data.shadowStrength = shadowFade * shadowSettings.shadowStrengthMultiplier;
				}
			}

		public:
			inline SpotLightData(uint32_t typeId,
				SceneContext* context,
				const RendererFrustrumDescriptor* frustrum,
				const EventObject<const Data&, const ShadowSettings&, LightDescriptor::Set*>* onUpdate,
				const Graphics::ShaderClass::TextureSamplerBinding* noShadowTexture,
				const Data& lastData, const ShadowSettings& lastShadowSettings)
				: m_context(context)
				, m_frustrum(frustrum)
				, m_onUpdate(onUpdate)
				, m_noShadowTexture(noShadowTexture) {
				m_info.typeId = typeId;
				m_info.data = &m_data;
				m_info.dataSize = sizeof(Data);
				Update(lastData, lastShadowSettings, nullptr);
				m_onUpdate->OnTick() += Callback(&SpotLightData::Update, this);
			}
			inline virtual ~SpotLightData() {
				m_onUpdate->OnTick() -= Callback(&SpotLightData::Update, this);
				Update(m_data, {}, nullptr);
				assert(m_shadowmapper == nullptr);
			}

			virtual LightDescriptor::LightInfo GetLightInfo()const override { return m_info; }

			virtual AABB GetLightBounds()const override {
				AABB bounds = {};
				bounds.start = m_data.position - Vector3(m_data.range);
				bounds.end = m_data.position + Vector3(m_data.range);
				return bounds;
			}
		};

		class SpotLightDescriptor 
			: public virtual LightDescriptor
			, public virtual ObjectCache<Reference<const Object>>
			, public virtual JobSystem::Job {
		public:
			SpotLight* m_owner;

		private:
			const Reference<SceneContext> m_context;
			const uint32_t m_typeId;
			const Reference<const Graphics::ShaderClass::TextureSamplerBinding> m_noShadowTexture;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_texture;

			Data m_data;
			ShadowSettings m_shadowSettings;

			const Reference<EventObject<const Data&, const ShadowSettings&, LightDescriptor::Set*>> m_onUpdate =
				Object::Instantiate<EventObject<const Data&, const ShadowSettings&, LightDescriptor::Set*>>();

			inline void UpdateData() {
				// Transformation:
				{
					const Transform* transform = m_owner->GetTransfrom();
					if (transform == nullptr) {
						m_data.position = Vector3(0.0f);
						m_data.forward = Math::Forward();
						m_data.up = Math::Up();
						m_shadowSettings.poseMatrix = Math::Identity();
					}
					else {
						m_data.position = transform->WorldPosition();
						Matrix4 poseMatrix = transform->WorldRotationMatrix();
						m_data.forward = poseMatrix[2];
						m_data.up = poseMatrix[1];
						poseMatrix[3] = Vector4(m_data.position, 1.0f);
						m_shadowSettings.poseMatrix = Math::Inverse(poseMatrix);
					}
				}
				
				// Spotlight shape:
				{
					m_shadowSettings.coneAngle = m_owner->OuterAngle();
					m_data.range = m_owner->Range();
					m_data.fadeAngleInvTangent = 1.0f / std::tan(Math::Radians(m_shadowSettings.coneAngle));
					m_data.spotThreshold = std::tan(Math::Radians(m_owner->InnerAngle())) * m_data.fadeAngleInvTangent;
				}

				// 'Projection color' sampler index:
				{
					if (m_texture == nullptr || m_texture->BoundObject() != m_owner->Texture()) {
						if (m_owner->Texture() == nullptr) {
							if (m_texture == nullptr || m_texture->BoundObject() != m_noShadowTexture->BoundObject())
								m_texture = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(m_noShadowTexture->BoundObject());
						}
						else m_texture = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(m_owner->Texture());
					}
					m_data.colorSamplerId = m_texture->Index();
				}

				// Spotlight color, tiling and offset:
				{
					m_data.colorTiling = m_owner->TextureTiling();
					m_data.colorOffset = m_owner->TextureOffset();
					m_data.baseColor = m_owner->Color() * m_owner->Intensity();
				}

				// Shadow settings:
				{
					m_shadowSettings.shadowResolution = m_owner->ShadowResolution();
					m_shadowSettings.shadowDistance = m_owner->ShadowDistance();
					m_shadowSettings.shadowFadeDistance = m_owner->ShadowFadeDistance();
					m_shadowSettings.shadowStrengthMultiplier = 1.0f;
					m_shadowSettings.softness = m_owner->ShadowSoftness();
					m_shadowSettings.filterSize = m_owner->ShadowFilterSize();
				}
			}

		public:
			inline SpotLightDescriptor(SpotLight* owner, uint32_t typeId) 
				: m_owner(owner), m_context(owner->Context()), m_typeId(typeId)
				, m_noShadowTexture(Graphics::ShaderClass::SharedTextureSamplerBinding(Vector4(1.0f), owner->Context()->Graphics()->Device())) {
				UpdateData();
			}


			// LightDescriptor:
			inline virtual Reference<const LightDescriptor::ViewportData> GetViewportData(const ViewportDescriptor* desc)override {
				return GetCachedOrCreate(desc, [&]() {
					return Object::Instantiate<SpotLightData>(m_typeId, m_context, desc, m_onUpdate, m_noShadowTexture, m_data, m_shadowSettings);
					});
			}


		protected:
			// JobSystem::Job:
			virtual void Execute()override {
				if (m_owner == nullptr) 
					return;
				UpdateData();
				m_onUpdate->Tick(m_data, m_shadowSettings, m_owner->m_allLights);
			}
			virtual void CollectDependencies(Callback<Job*>)override {}
		};

#pragma warning(default: 4250)
	};

	SpotLight::SpotLight(Component* parent, const std::string_view& name)
		: Component(parent, name)
		, m_allLights(LightDescriptor::Set::GetInstance(parent->Context())) {}

	SpotLight::~SpotLight() {
		OnComponentDisabled();
	}

	void SpotLight::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Range, SetRange, "Range", "Maximal distance, the SpotLight will illuminate at");
			JIMARA_SERIALIZE_FIELD_GET_SET(InnerAngle, SetInnerAngle, "Inner Angle", "Projection cone angle, before the intencity starts fading out",
				Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 90.0f));
			JIMARA_SERIALIZE_FIELD_GET_SET(OuterAngle, SetOuterAngle, "Outer Angle", "Projection cone angle at which the intencity will drop to zero",
				Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 90.0f));
			JIMARA_SERIALIZE_FIELD_GET_SET(Color, SetColor, "Color", "Base color of spotlight emission", Object::Instantiate<Serialization::ColorAttribute>());
			JIMARA_SERIALIZE_FIELD_GET_SET(Intensity, SetIntensity, "Intensity", "Color multiplier");
			JIMARA_SERIALIZE_FIELD_GET_SET(Texture, SetTexture, "Texture", "Optionally, the spotlight projection can take color form this texture");
			if (Texture() != nullptr) {
				JIMARA_SERIALIZE_FIELD_GET_SET(TextureTiling, SetTextureTiling, "Tiling", "Tells, how many times should the texture repeat itself");
				JIMARA_SERIALIZE_FIELD_GET_SET(TextureOffset, SetTextureOffset, "Offset", "Tells, how to shift the texture around");
			}
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
				JIMARA_SERIALIZE_FIELD_GET_SET(ShadowDistance, SetShadowDistance, "Shadow Distance",
					"Shadow distance from viewport origin, before it starts fading");
				JIMARA_SERIALIZE_FIELD_GET_SET(ShadowFadeDistance, SetShadowFadeDistance, "Shadow Fade Distance",
					"Shadow fade-out distance after ShadowDistance, before it fully disapears");
			}
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
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<SpotLight>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<SpotLight>(
			"Spot Light", "Jimara/Lights/SpotLight", "Cone-shaped light emitter");
		report(factory);
	}
}
