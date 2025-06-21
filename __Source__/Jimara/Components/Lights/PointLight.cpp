#include "PointLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include "../../Data/Serialization/Attributes/SliderAttribute.h"
#include "../../Environment/Rendering/LightingModels/DepthOnlyRenderer/DualParaboloidDepthRenderer.h"
#include "../../Environment/Rendering/Shadows/VarianceShadowMapper/VarianceShadowMapper.h"
#include "../../Environment/Rendering/TransientImage.h"
#include "../../Core/Stopwatch.h"


namespace Jimara {
	struct PointLight::Helpers {
		struct ShadowMapper : public virtual LightmapperJobs::Job {
			const Reference<SceneContext> context;
			const Reference<DualParaboloidDepthRenderer> depthRenderer;
			const Reference<VarianceShadowMapper> varianceMapGenerator;
			float timeLeft = 0.0f;

			inline ShadowMapper(SceneContext* context, const RendererFrustrumDescriptor* rendererFrustrum)
				: context(context)
				, depthRenderer(Object::Instantiate<DualParaboloidDepthRenderer>(context, LayerMask::All(), rendererFrustrum, RendererFrustrumFlags::SHADOWMAPPER))
				, varianceMapGenerator(VarianceShadowMapper::Create(context)) {}

			inline virtual ~ShadowMapper() {}

			inline virtual void Execute() override {
				const Graphics::InFlightBufferInfo commandBufferInfo = context->Graphics()->GetWorkerThreadCommandBuffer();
				depthRenderer->Render(commandBufferInfo);
				varianceMapGenerator->GenerateVarianceMap(commandBufferInfo);
			}
			inline virtual void CollectDependencies(Callback<JobSystem::Job*> record) override {
				depthRenderer->GetDependencies(record);
			}
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
				return Object::Instantiate<ShadowMapper>(m_context, m_frustrum);
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


		struct LightData {
			// Transform:
			alignas(16) Vector3 position = Vector3(0.0f);	// Bytes [0 - 12)	Transform::Position();

			// Color:
			alignas(16) Vector3 color = Vector3(1.0f);		// Bytes [16 - 28)	Color() * Intensity();

			// Shadow & Range:
			alignas(4) float inverseRange = 0.1f;			// Bytes [28 - 32)	Radius();
			alignas(4) float depthEpsilon = 0.005f;			// Bytes [32 - 36)	Error margin for elleminating shimmering caused by floating point inaccuracies from the depth map.
			alignas(4) float zEpsilon = 0.0f;				// Bytes [36 - 40)	Z-epsilon for shadow sampling.
			alignas(4) uint32_t shadowSamplerId = 0u;		// Bytes [40 - 44) 	BindlessSamplers::GetFor(shadowTexture).Index();
			alignas(4) float shadowStrength = 0.0f;			// Bytes [44 - 48)	Multiplier for shadowmap strength
		};
		static_assert(sizeof(LightData) == 48);

		struct ShadowSettings {
			uint32_t shadowResolution = 0u;
			float shadowDistance = 0.0f;
			float shadowFadeDistance = 0.0f;
			float shadowStrengthMultiplier = 1.0f;
			float radius = 0.0f;
			float softness = 0.0f;
			uint32_t filterSize = 1u;
		};

		inline static const constexpr float ClosePlane() { return 0.01f; }

		class PointLightData 
			: public virtual LightDescriptor::ViewportData
			, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<const RendererFrustrumDescriptor> m_frustrum;
			const Reference<const EventObject<const LightData&, const ShadowSettings&, LightDescriptor::Set*>> m_onUpdate;
			
			LightData m_data = {};
			LightDescriptor::LightInfo m_info = {};

			Reference<ViewportShadowmapperCache> m_viewportShadowmappers;
			Reference<ShadowMapper> m_shadowmapper;

			Reference<LightmapperJobs> m_lightmapperJobs;
			Reference<LightmapperJobs::ItemOwner> m_shadowRenderJob;
			Reference<Graphics::TextureSampler> m_shadowTexture;
			Reference<const TransientImage> m_depthTexture;

			const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> m_noShadowTexture;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_shadowSamplerId;

			void Update(const LightData& curData, const ShadowSettings& shadowSettings, LightDescriptor::Set* allLights) {
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
					const Size3 textureSize = Size3(shadowSettings.shadowResolution * 2u, shadowSettings.shadowResolution, 1u);
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
						m_shadowTexture = m_shadowmapper->varianceMapGenerator->SetDepthTexture(sampler);
					}

					// Update shadowmapper settings:
					m_shadowmapper->depthRenderer->Configure(m_data.position, ClosePlane(), shadowSettings.radius);
					m_shadowmapper->varianceMapGenerator->Configure(ClosePlane(), shadowSettings.radius, shadowSettings.softness, shadowSettings.filterSize);
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
			inline PointLightData(uint32_t typeId,
				SceneContext* context,
				const RendererFrustrumDescriptor* frustrum, 
				const EventObject<const LightData&, const ShadowSettings&, LightDescriptor::Set*>* onUpdate,
				const Graphics::ResourceBinding<Graphics::TextureSampler>* noShadowTexture,
				const LightData& lastData, const ShadowSettings& lastShadowSettings)
				: m_context(context)
				, m_frustrum(frustrum)
				, m_onUpdate(onUpdate)
				, m_noShadowTexture(noShadowTexture) {
				m_info.typeId = typeId;
				m_info.data = &m_data;
				m_info.dataSize = sizeof(LightData);
				Update(lastData, lastShadowSettings, nullptr);
				m_onUpdate->OnTick() += Callback(&PointLightData::Update, this);
			}
			inline virtual ~PointLightData() {
				m_onUpdate->OnTick() -= Callback(&PointLightData::Update, this);
				Update(m_data, {}, nullptr);
				assert(m_shadowmapper == nullptr);
			}

			virtual LightDescriptor::LightInfo GetLightInfo()const override { return m_info; }

			virtual AABB GetLightBounds()const override {
				AABB bounds = {};
				const float radius = 1.0f / Math::Max(m_data.inverseRange, std::numeric_limits<float>::epsilon()) + 0.001f;
				bounds.start = m_data.position - radius;
				bounds.end = m_data.position + radius;
				return bounds;
			}
		};

		class PointLightDescriptor 
			: public virtual LightDescriptor
			, public virtual ObjectCache<Reference<const Object>> {
		public:
			PointLight* m_owner;

		private:
			const Reference<SceneContext> m_context;
			const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> m_noShadowTexture;
			const uint32_t m_typeId;

			LightData m_data;
			ShadowSettings m_shadowSettings;

			const Reference<EventObject<const LightData&, const ShadowSettings&, LightDescriptor::Set*>> m_onUpdate =
				Object::Instantiate<EventObject<const LightData&, const ShadowSettings&, LightDescriptor::Set*>>();

			void UpdateData(const LocalLightShadowSettings* shadowSettings) {
				if (m_owner == nullptr) 
					return;

				// Transform:
				Matrix4 worldMatrix;
				{
					const Transform* transform = m_owner->GetTransfrom();
					worldMatrix = (transform == nullptr) ? Math::Identity() : transform->FrameCachedWorldMatrix();
					m_data.position = worldMatrix[3u];
				}

				// Color and range:
				{
					m_data.color = m_owner->Color() * m_owner->Intensity();
					m_data.inverseRange = 1.0f / Math::Max(m_owner->Radius(), std::numeric_limits<float>::epsilon());

					const float scale = 4.0f * static_cast<float>(Math::Max(shadowSettings->ShadowResolution(), 1u)) / 512.0f;
					const float filterSize = static_cast<float>(shadowSettings->ShadowFilterSize());
					const float invSoftness = 1.0f - shadowSettings->ShadowSoftness();
					m_data.zEpsilon = ClosePlane() * ((filterSize * (1.0f - (invSoftness * invSoftness))) / scale + 1.0f);
				}
			}

			void UpdateShadowSettings(const LocalLightShadowSettings* shadowSettings) {
				if (m_owner == nullptr) 
					return;
				m_shadowSettings.shadowResolution = shadowSettings->ShadowResolution();
				m_shadowSettings.shadowDistance = shadowSettings->ShadowDistance();
				m_shadowSettings.shadowFadeDistance = shadowSettings->ShadowFadeDistance();
				m_shadowSettings.shadowStrengthMultiplier = 1.0f;
				m_shadowSettings.radius = m_owner->Radius();
				m_shadowSettings.softness = shadowSettings->ShadowSoftness();
				m_shadowSettings.filterSize = shadowSettings->ShadowFilterSize();
			}

		public:
			inline PointLightDescriptor(PointLight* owner, uint32_t typeId)
				: m_owner(owner), m_context(owner->Context())
				, m_noShadowTexture(Graphics::SharedTextureSamplerBinding(Vector4(0.0f, 0.0f, 0.0f, 1.0f), owner->Context()->Graphics()->Device()))
				, m_typeId(typeId) {
				Reference<const LocalLightShadowSettings> shadowSettings = LocalLightShadowSettingsProvider::GetInput(m_owner->m_shadowSettings, nullptr);
				if (shadowSettings == nullptr)
					shadowSettings = &m_owner->m_defaultShadowSettings;
				UpdateData(shadowSettings);
				UpdateShadowSettings(shadowSettings);
			}

			virtual Reference<const LightDescriptor::ViewportData> GetViewportData(const ViewportDescriptor* desc)override { 
				return GetCachedOrCreate(desc, [&]() {
					return Object::Instantiate<PointLightData>(m_typeId, m_context, desc, m_onUpdate, m_noShadowTexture, m_data, m_shadowSettings);
					});
			}

			inline void Update(LightDescriptor::Set* allLights) {
				if (m_owner == nullptr)
					return;
				Reference<const LocalLightShadowSettings> shadowSettings = LocalLightShadowSettingsProvider::GetInput(m_owner->m_shadowSettings, nullptr);
				if (shadowSettings == nullptr)
					shadowSettings = &m_owner->m_defaultShadowSettings;
				UpdateData(shadowSettings);
				UpdateShadowSettings(shadowSettings);
				m_onUpdate->Tick(m_data, m_shadowSettings, allLights);
			}
		};
#pragma warning(default: 4250)


		struct PointLightList : public virtual JobSystem::Job {
			const Reference<LightDescriptor::Set> allLights;
			std::mutex lock;
			DelayedObjectSet<PointLightDescriptor> descriptors;

			inline PointLightList(SceneContext* context) 
				: allLights(LightDescriptor::Set::GetInstance(context)) {}

			inline virtual ~PointLightList() {}

			virtual void Execute()override {
				std::unique_lock<decltype(lock)> flushLock(lock);
				descriptors.Flush([](const auto...) {}, [](const auto...) {});
			}

			virtual void CollectDependencies(Callback<Job*>)override {}
		};

		class PointLightUpdateJob : public virtual JobSystem::Job {
		public:
			const size_t m_index;
			const size_t m_updaterCount;
			const Reference<PointLightList> m_pointLightList;

		public:
			inline PointLightUpdateJob(size_t index, size_t jobCount, PointLightList* lightList) 
				: m_index(index), m_updaterCount(jobCount), m_pointLightList(lightList) { }

			inline virtual ~PointLightUpdateJob() {}

		protected:
			virtual void Execute()override {
				const size_t descriptorCount = m_pointLightList->descriptors.Size();
				const size_t descriptorsPerJob = (descriptorCount + m_updaterCount - 1u) / m_updaterCount;
				const Reference<PointLightDescriptor>* const descriptors = m_pointLightList->descriptors.Data();
				const Reference<PointLightDescriptor>* const first = descriptors + (descriptorsPerJob * m_index);
				const Reference<PointLightDescriptor>* const last = Math::Min(first + descriptorsPerJob, descriptors + descriptorCount);
				LightDescriptor::Set* const allLights = m_pointLightList->allLights;
				for (const Reference<PointLightDescriptor>* ptr = first; ptr < last; ptr++)
					(*ptr)->Update(allLights);
			}

			virtual void CollectDependencies(Callback<Job*> report)override { report(m_pointLightList); }
		};

		class PointLightJobs : public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<PointLightList> m_lightList;
			std::vector<Reference<PointLightUpdateJob>> m_updateJobs;

		public:
			inline PointLightJobs(SceneContext* context) 
				: m_context(context)
				, m_lightList(Object::Instantiate<PointLightList>(context)) {
				const size_t numJobs = Math::Max((size_t)std::thread::hardware_concurrency(), (size_t)1u);
				for (size_t i = 0u; i < numJobs; i++) {
					const Reference<PointLightUpdateJob> job = Object::Instantiate<PointLightUpdateJob>(i, numJobs, m_lightList);
					m_context->Graphics()->SynchPointJobs().Add(job);
					m_updateJobs.push_back(job);
				}
			}

			inline LightDescriptor::Set* AllLights()const { return m_lightList->allLights; }

			inline virtual ~PointLightJobs() {
				for (size_t i = 0u; i < m_updateJobs.size(); i++)
					m_context->Graphics()->SynchPointJobs().Remove(m_updateJobs[i]);
			}

			inline void Add(PointLightDescriptor* desc) {
				std::unique_lock<decltype(m_lightList->lock)> flushLock(m_lightList->lock);
				m_lightList->descriptors.ScheduleAdd(desc);
			}

			inline void Remove(PointLightDescriptor* desc) {
				std::unique_lock<decltype(m_lightList->lock)> flushLock(m_lightList->lock);
				m_lightList->descriptors.ScheduleRemove(desc);
			}

			static Reference<PointLightJobs> Instance(SceneContext* context) {
				if (context == nullptr)
					return nullptr;
				struct Cache : public virtual ObjectCache<Reference<const Object>> {
					inline Reference<PointLightJobs> GetFor(SceneContext* context) {
						return GetCachedOrCreate(context, [&]() -> Reference<ObjectCache<Reference<const Object>>::StoredObject> {
							return Object::Instantiate<PointLightJobs>(context);
							});
					}
				};
				static Cache cache;
				return cache.GetFor(context);
			}
		};

		inline static void OnEnabledOrDisabled(PointLight* self) {
			PointLightJobs* const allDescriptors = dynamic_cast<PointLightJobs*>(self->m_allLights.operator->());
			LightDescriptor::Set* const allLights = allDescriptors->AllLights();

			if (!self->ActiveInHierarchy()) {
				if (self->m_lightDescriptor == nullptr)
					return;
				allLights->Remove(self->m_lightDescriptor);
				allDescriptors->Remove(dynamic_cast<Helpers::PointLightDescriptor*>(self->m_lightDescriptor->Item()));
				dynamic_cast<Helpers::PointLightDescriptor*>(self->m_lightDescriptor->Item())->m_owner = nullptr;
				self->m_lightDescriptor = nullptr;
				if (self->Destroyed())
					self->m_allLights = nullptr;
			}
			else if (self->m_lightDescriptor == nullptr) {
				uint32_t typeId;
				if (self->Context()->Graphics()->Configuration().ShaderLibrary()->GetLightTypeId("Jimara_PointLight", typeId)) {
					Reference<Helpers::PointLightDescriptor> descriptor = Object::Instantiate<Helpers::PointLightDescriptor>(self, typeId);
					self->m_lightDescriptor = Object::Instantiate<LightDescriptor::Set::ItemOwner>(descriptor);
					allLights->Add(self->m_lightDescriptor);
					allDescriptors->Add(descriptor);
				}
			}
		}
	};

	PointLight::PointLight(Component* parent, const std::string_view& name, Vector3 color, float radius)
		: Component(parent, name)
		, m_allLights(Helpers::PointLightJobs::Instance(parent->Context()))
		, m_color(color)
		, m_radius(radius) {}

	PointLight::~PointLight() {
		Helpers::OnEnabledOrDisabled(this);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<PointLight>(const Callback<const Object*>& report) { 
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<PointLight>(
			"Point Light", "Jimara/Lights/PointLight", "Point-like light source");
		report(factory);
	}


	Vector3 PointLight::Color()const { return m_color; }

	void PointLight::SetColor(const Vector3& color) { m_color = color; }


	float PointLight::Radius()const { return m_radius; }

	void PointLight::SetRadius(float radius) { m_radius = radius <= 0.0f ? 0.0f : radius; }

	void PointLight::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Color, SetColor, "Color", "Light Color", Object::Instantiate<Serialization::ColorAttribute>());
			JIMARA_SERIALIZE_FIELD_GET_SET(Intensity, SetIntensity, "Intensity", "Color multiplier");
			JIMARA_SERIALIZE_FIELD_GET_SET(Radius, SetRadius, "Radius", "Maximal illuminated distance");
			JIMARA_SERIALIZE_FIELD(m_shadowSettings, "Shadow Settings", "Shadow Settings provider");
			const Reference<LocalLightShadowSettingsProvider> shadowSettings = m_shadowSettings;
			if (shadowSettings == nullptr)
				m_defaultShadowSettings.GetFields(recordElement);
		};
	}

	void PointLight::GetSerializedActions(Callback<Serialization::SerializedCallback> report) {
		Component::GetSerializedActions(report);

		// Color:
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector3>::Create(
				"Color", "Light color", std::vector<Reference<const Object>> { Object::Instantiate<Serialization::ColorAttribute>() });
			report(Serialization::SerializedCallback::Create<const Vector3&>::From(
				"SetColor", Callback<const Vector3&>(&PointLight::SetColor, this), serializer));
		}

		// Intensity:
		{
			static const auto serializer = Serialization::DefaultSerializer<float>::Create("Intensity", "Color multiplier");
			report(Serialization::SerializedCallback::Create<float>::From(
				"SetIntensity", Callback<float>(&PointLight::SetIntensity, this), serializer));
		}

		// Intensity:
		{
			static const auto serializer = Serialization::DefaultSerializer<float>::Create("Radius", "Maximal illuminated distance");
			report(Serialization::SerializedCallback::Create<float>::From(
				"SetRadius", Callback<float>(&PointLight::SetRadius, this), serializer));
		}

		// Shadow settings:
		{
			static const auto serializer = Serialization::DefaultSerializer<LocalLightShadowSettingsProvider*>::Create(
				"Shadow Settings", "Shadow Settings provider");
			report(Serialization::SerializedCallback::Create<LocalLightShadowSettingsProvider*>::From(
				"SetShadowSettings", Callback<LocalLightShadowSettingsProvider*>(&PointLight::SetShadowSettings, this), serializer));
		}
	}

	void PointLight::OnComponentInitialized() {
		Helpers::OnEnabledOrDisabled(this);
	}

	void PointLight::OnComponentEnabled() {
		Helpers::OnEnabledOrDisabled(this);
	}

	void PointLight::OnComponentDisabled() {
		Helpers::OnEnabledOrDisabled(this);
	}
}
