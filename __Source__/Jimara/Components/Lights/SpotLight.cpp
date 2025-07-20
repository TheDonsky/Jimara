#include "SpotLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include "../../Data/Serialization/Attributes/SliderAttribute.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Environment/Rendering/LightingModels/DepthOnlyRenderer/DepthOnlyRenderer.h"
#include "../../Environment/Rendering/Shadows/VarianceShadowMapper/VarianceShadowMapper.h"
#include "../../Environment/Rendering/TransientImage.h"
#include "../../Core/BulkAllocated.h"
#include "../../Core/Stopwatch.h"


namespace Jimara {
	struct SpotLight::Helpers {
		struct Viewport : public virtual ViewportDescriptor {
			Matrix4 viewMatrix = Math::Identity();
			Matrix4 projectionMatrix = Math::Identity();
			const Reference<const RendererFrustrumDescriptor> mainViewport;

			inline Viewport(SceneContext* context, const RendererFrustrumDescriptor* mainView) 
				: RendererFrustrumDescriptor(RendererFrustrumFlags::SHADOWMAPPER)
				, ViewportDescriptor(context), mainViewport(mainView) {}
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
			, public virtual ObjectCache<Reference<const Object>>::StoredObject
			, public virtual BulkAllocated {
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

			const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> m_noShadowTexture;
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
				const Graphics::ResourceBinding<Graphics::TextureSampler>* noShadowTexture,
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
			, public virtual ::Jimara::BulkAllocated {
		public:
			SpotLight* m_owner;

		private:
			const Reference<SceneContext> m_context;
			const uint32_t m_typeId;
			const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> m_noShadowTexture;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_texture;

			Data m_data;
			ShadowSettings m_shadowSettings;

			const Reference<EventObject<const Data&, const ShadowSettings&, LightDescriptor::Set*>> m_onUpdate =
				Object::Instantiate<EventObject<const Data&, const ShadowSettings&, LightDescriptor::Set*>>();

			inline void UpdateData() {
				// Transformation:
				{
					const Transform* transform = m_owner->GetTransform();
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
					Reference<const LocalLightShadowSettings> shadowSettings = LocalLightShadowSettingsProvider::GetInput(m_owner->m_shadowSettings, nullptr);
					if (shadowSettings == nullptr)
						shadowSettings = m_owner->m_defaultShadowSettings;
					m_shadowSettings.shadowResolution = shadowSettings->ShadowResolution();
					m_shadowSettings.shadowDistance = shadowSettings->ShadowDistance();
					m_shadowSettings.shadowFadeDistance = shadowSettings->ShadowFadeDistance();
					m_shadowSettings.shadowStrengthMultiplier = 1.0f;
					m_shadowSettings.softness = shadowSettings->ShadowSoftness();
					m_shadowSettings.filterSize = shadowSettings->ShadowFilterSize();
				}
			}

		public:
			inline SpotLightDescriptor(SpotLight* owner, uint32_t typeId) 
				: m_owner(owner), m_context(owner->Context()), m_typeId(typeId)
				, m_noShadowTexture(Graphics::SharedTextureSamplerBinding(Vector4(1.0f), owner->Context()->Graphics()->Device())) {
				UpdateData();
			}


			// LightDescriptor:
			inline virtual Reference<const LightDescriptor::ViewportData> GetViewportData(const ViewportDescriptor* desc)override {
				return GetCachedOrCreate(desc, [&]() {
					return BulkAllocated::Allocate<SpotLightData>(m_typeId, m_context, desc, m_onUpdate, m_noShadowTexture, m_data, m_shadowSettings);
					});
			}

			// Update:
			inline void Update(LightDescriptor::Set* allLights) {
				if (m_owner == nullptr)
					return;
				UpdateData();
				m_onUpdate->Tick(m_data, m_shadowSettings, allLights);
			}


		protected:
			virtual inline void OnOutOfScope()const override {
				BulkAllocated::OnOutOfScope();
			}
		};

#pragma warning(default: 4250)


		struct SpotLightList : public virtual JobSystem::Job {
			const Reference<LightDescriptor::Set> allLights;
			std::mutex lock;
			DelayedObjectSet<SpotLightDescriptor> descriptors;

			inline SpotLightList(SceneContext* context)
				: allLights(LightDescriptor::Set::GetInstance(context)) {
			}

			inline virtual ~SpotLightList() {}

			virtual void Execute()override {
				std::unique_lock<decltype(lock)> flushLock(lock);
				descriptors.Flush([](const auto...) {}, [](const auto...) {});
			}

			virtual void CollectDependencies(Callback<Job*>)override {}
		};

		class SpotLightUpdateJob : public virtual JobSystem::Job {
		public:
			const size_t m_index;
			const size_t m_updaterCount;
			const Reference<SpotLightList> m_lightList;

		public:
			inline SpotLightUpdateJob(size_t index, size_t jobCount, SpotLightList* lightList)
				: m_index(index), m_updaterCount(jobCount), m_lightList(lightList) {
			}

			inline virtual ~SpotLightUpdateJob() {}

		protected:
			virtual void Execute()override {
				const size_t descriptorCount = m_lightList->descriptors.Size();
				const size_t descriptorsPerJob = (descriptorCount + m_updaterCount - 1u) / m_updaterCount;
				const Reference<SpotLightDescriptor>* const descriptors = m_lightList->descriptors.Data();
				const Reference<SpotLightDescriptor>* const first = descriptors + (descriptorsPerJob * m_index);
				const Reference<SpotLightDescriptor>* const last = Math::Min(first + descriptorsPerJob, descriptors + descriptorCount);
				LightDescriptor::Set* const allLights = m_lightList->allLights;
				for (const Reference<SpotLightDescriptor>* ptr = first; ptr < last; ptr++)
					(*ptr)->Update(allLights);
			}

			virtual void CollectDependencies(Callback<Job*> report)override { report(m_lightList); }
		};

		class SpotLightJobs : public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<SpotLightList> m_lightList;
			std::vector<Reference<SpotLightUpdateJob>> m_updateJobs;

		public:
			inline SpotLightJobs(SceneContext* context)
				: m_context(context)
				, m_lightList(Object::Instantiate<SpotLightList>(context)) {
				const size_t numJobs = Math::Max((size_t)std::thread::hardware_concurrency(), (size_t)1u);
				for (size_t i = 0u; i < numJobs; i++) {
					const Reference<SpotLightUpdateJob> job = Object::Instantiate<SpotLightUpdateJob>(i, numJobs, m_lightList);
					m_context->Graphics()->SynchPointJobs().Add(job);
					m_updateJobs.push_back(job);
				}
			}

			inline LightDescriptor::Set* AllLights()const { return m_lightList->allLights; }

			inline virtual ~SpotLightJobs() {
				for (size_t i = 0u; i < m_updateJobs.size(); i++)
					m_context->Graphics()->SynchPointJobs().Remove(m_updateJobs[i]);
			}

			inline void Add(SpotLightDescriptor* desc) {
				std::unique_lock<decltype(m_lightList->lock)> flushLock(m_lightList->lock);
				m_lightList->descriptors.ScheduleAdd(desc);
			}

			inline void Remove(SpotLightDescriptor* desc) {
				std::unique_lock<decltype(m_lightList->lock)> flushLock(m_lightList->lock);
				m_lightList->descriptors.ScheduleRemove(desc);
			}

			static Reference<SpotLightJobs> Instance(SceneContext* context) {
				if (context == nullptr)
					return nullptr;
				struct Cache : public virtual ObjectCache<Reference<const Object>> {
					inline Reference<SpotLightJobs> GetFor(SceneContext* context) {
						return GetCachedOrCreate(context, [&]() -> Reference<ObjectCache<Reference<const Object>>::StoredObject> {
							return Object::Instantiate<SpotLightJobs>(context);
							});
					}
				};
				static Cache cache;
				return cache.GetFor(context);
			}
		};

		inline static void OnEnabledOrDisabled(SpotLight* self) {
			SpotLightJobs* const allDescriptors = dynamic_cast<SpotLightJobs*>(self->m_allLights.operator->());
			LightDescriptor::Set* const allLights = allDescriptors->AllLights();

			if (!self->ActiveInHierarchy()) {
				if (self->m_lightDescriptor == nullptr)
					return;
				allLights->Remove(self->m_lightDescriptor);
				allDescriptors->Remove(dynamic_cast<Helpers::SpotLightDescriptor*>(self->m_lightDescriptor->Item()));
				dynamic_cast<Helpers::SpotLightDescriptor*>(self->m_lightDescriptor->Item())->m_owner = nullptr;
				self->m_lightDescriptor = nullptr;
				if (self->Destroyed())
					self->m_allLights = nullptr;
			}
			else if (self->m_lightDescriptor == nullptr) {
				uint32_t typeId;
				if (self->Context()->Graphics()->Configuration().ShaderLibrary()->GetLightTypeId("Jimara_SpotLight", typeId)) {
					Reference<Helpers::SpotLightDescriptor> descriptor = BulkAllocated::Allocate<Helpers::SpotLightDescriptor>(self, typeId);
					self->m_lightDescriptor = Object::Instantiate<LightDescriptor::Set::ItemOwner>(descriptor);
					allLights->Add(self->m_lightDescriptor);
					allDescriptors->Add(descriptor);
				}
			}
		}
	};

	SpotLight::SpotLight(Component* parent, const std::string_view& name)
		: Component(parent, name)
		, m_allLights(Helpers::SpotLightJobs::Instance(parent->Context())) {}

	SpotLight::~SpotLight() {
		Helpers::OnEnabledOrDisabled(this);
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
			JIMARA_SERIALIZE_FIELD(m_shadowSettings, "Shadow Settings", "Shadow Settings provider");
			const Reference<LocalLightShadowSettingsProvider> shadowSettings = m_shadowSettings;
			if (shadowSettings == nullptr)
				m_defaultShadowSettings->GetFields(recordElement);
		};
	}

	void SpotLight::GetSerializedActions(Callback<Serialization::SerializedCallback> report) {
		Component::GetSerializedActions(report);

		// Color:
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector3>::Create(
				"Color", "Light color", std::vector<Reference<const Object>> { Object::Instantiate<Serialization::ColorAttribute>() });
			report(Serialization::SerializedCallback::Create<const Vector3&>::From(
				"SetColor", Callback<const Vector3&>(&SpotLight::SetColor, this), serializer));
		}

		// Intensity:
		{
			static const auto serializer = Serialization::DefaultSerializer<float>::Create("Intensity", "Color multiplier");
			report(Serialization::SerializedCallback::Create<float>::From(
				"SetIntensity", Callback<float>(&SpotLight::SetIntensity, this), serializer));
		}

		// Inner Angle:
		{
			static const auto serializer = Serialization::DefaultSerializer<float>::Create(
				"Angle", "Projection cone angle, before the intencity starts fading out",
				std::vector<Reference<const Object>> { Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 90.0f) });
			report(Serialization::SerializedCallback::Create<float>::From(
				"SetInnerAngle", Callback<float>(&SpotLight::SetInnerAngle, this), serializer));
		}

		// Outer Angle:
		{
			static const auto serializer = Serialization::DefaultSerializer<float>::Create(
				"Outer Angle", "Projection cone angle at which the intencity will drop to zero",
				std::vector<Reference<const Object>> { Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 90.0f) });
			report(Serialization::SerializedCallback::Create<float>::From(
				"SetOuterAngle", Callback<float>(&SpotLight::SetOuterAngle, this), serializer));
		}

		// Textrue:
		{
			static const auto serializer = Serialization::DefaultSerializer<Graphics::TextureSampler*>::Create(
				"Texture", "Optionally, the spotlight projection can take color form this texture");
			report(Serialization::SerializedCallback::Create<Graphics::TextureSampler*>::From(
				"SetTexture", Callback<Graphics::TextureSampler*>(&SpotLight::SetTexture, this), serializer));
		}

		// Texture tiling:
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector2>::Create(
				"Tiling", "Tells, how many times should the texture repeat itself");
			report(Serialization::SerializedCallback::Create<const Vector2&>::From(
				"SetTextureTiling", Callback<const Vector2&>(&SpotLight::SetTextureTiling, this), serializer));
		}

		// Texture offset:
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector2>::Create(
				"Offset", "Tells, how to shift the texture around");
			report(Serialization::SerializedCallback::Create<const Vector2&>::From(
				"SetTextureOffset", Callback<const Vector2&>(&SpotLight::SetTextureOffset, this), serializer));
		}

		// Shadow settings:
		{
			static const auto serializer = Serialization::DefaultSerializer<LocalLightShadowSettingsProvider*>::Create(
				"Shadow Settings", "Shadow Settings provider");
			report(Serialization::SerializedCallback::Create<LocalLightShadowSettingsProvider*>::From(
				"SetShadowSettings", Callback<LocalLightShadowSettingsProvider*>(&SpotLight::SetShadowSettings, this), serializer));
		}
	}

	void SpotLight::OnComponentInitialized() {
		Helpers::OnEnabledOrDisabled(this);
	}

	void SpotLight::OnComponentEnabled() {
		Helpers::OnEnabledOrDisabled(this);
	}

	void SpotLight::OnComponentDisabled() {
		Helpers::OnEnabledOrDisabled(this);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<SpotLight>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<SpotLight>(
			"Spot Light", "Jimara/Lights/SpotLight", "Cone-shaped light emitter");
		report(factory);
	}
}
