#include "DirectionalLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include "../../Data/Serialization/Attributes/SliderAttribute.h"
#include "../../Environment/Rendering/LightingModels/DepthOnlyRenderer/DepthOnlyRenderer.h"
#include "../../Environment/Rendering/Shadows/VarianceShadowMapper/VarianceShadowMapper.h"
#include "../../Environment/Rendering/TransientImage.h"
#include <limits>


namespace Jimara {
	struct DirectionalLight::Helpers {
		// BUFFERS:
		struct Jimara_DirectionalLight_CascadeInfo {
			alignas(8) Vector2 lightmapOffset = Vector2(0.0f);	// Bytes [0 - 8)	Lightmap UV offset (center (X, Y coordinates) * lightmapSize + 0.5f) in "light space"
			alignas(4) float lightmapSize = 1.0f;				// Bytes [8 - 12)	Lightmap orthographic size
			alignas(4) float lightmapDepth = 0.0f;				// Bytes [12 - 16)	Inversed Z coordinate of the lightmap's view matrix in "light space"
			alignas(4) float inverseFarPlane = 0.0f;			// Bytes [16 - 20)	1.0f / farPlane
			alignas(4) float viewportDistance = 0.0f;			// Bytes [20 - 24)	Maximal distance from viewport, this lightmap will cover
			alignas(4) float blendDistance = 0.0f;				// Bytes [24 - 28)	Blended region size between this cascade and the next one (fade size for the last cascade)
			alignas(4) uint32_t shadowSamplerId = 0;			// Bytes [28 - 32)	Sampler index in the global bindless array
		};
		static_assert(sizeof(Jimara_DirectionalLight_CascadeInfo) == 32);

		struct Jimara_DirectionalLight_Data {
			alignas(16) Vector3 up = Math::Up();					// Bytes [0 - 12)	lightRotation.up
			alignas(4) uint32_t textureTiling = 0u;					// Bytes [12 - 16)	packHalf2x16(TextureTiling())
			alignas(16) Vector3 forward = Math::Forward();			// Bytes [16 - 28)	lightRotation.forward
			alignas(4) uint32_t textureOffset = 0u;					// Bytes [28 - 32)	packHalf2x16(TextureOffset())
			alignas(16) Vector3 viewportForward = Math::Forward();	// Bytes [32 - 44)	viewMatrix.forward
			alignas(4) uint32_t numCascadesAndAmbientAmount = 0u;	// Bytes [44 - 48)	packHalf2x16(Vector2(ShadowCascadeCount() + 0.5f, ShadowStrength()))
			alignas(16) Vector3 color = Vector3(1.0f);				// Bytes [48 - 60)	Color() * Intensity()
			alignas(4) uint32_t colorTextureId = 0u;				// Bytes [60 - 64)	Color sampler index

			Jimara_DirectionalLight_CascadeInfo cascades[4];		// Bytes [64 - 192)
		};
		static_assert(sizeof(Jimara_DirectionalLight_Data) == 192);





		// LIGHT SOURCE STETE SNAPSHOT:
		struct LightSourceState {
			struct TransformState {
				Matrix4 rotation = Math::Identity();
			} transform;

			struct ColorState {
				Vector3 baseColor = Vector3(1.0f);
				Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> texture;
				Vector2 textureTiling = Vector2(1.0f);
				Vector2 textureOffset = Vector2(0.0f);
			} color;

			struct ShadowState {
				float outOfFrustrumRange = 100.0f;
				float depthEpsilon = 0.1f;
				uint32_t resolution = 0u;
				float ambientAmount = 0.25f;
				float softness = 0.25f;
				uint32_t kernelSize = 5u;
				float shadowSizeMultiplier = 1.0f;
				Stacktor<ShadowCascadeInfo, 4u> cascades;
			} shadows;
		};


		// LIGHT SYNCH JOB/DESCRIPTOR:
		class DirectionalLightInfo : public virtual JobSystem::Job {
		private:
			const Reference<const Graphics::ShaderClass::TextureSamplerBinding> m_whiteTexture;
			const uint32_t m_typeId;
			mutable SpinLock m_ownerLock;
			DirectionalLight* m_owner;
			EventInstance<DirectionalLight*> m_onUpdate;
			LightSourceState m_lightState;

			inline void RemoveTemporaryLightDescriptor(DirectionalLight* owner) {
				if (owner->m_tmpLightDescriptor == nullptr) return;
				owner->m_allLights->Remove(owner->m_tmpLightDescriptor);
				owner->m_tmpLightDescriptor = nullptr;
			}

			inline void UpdateTemporaryLightDescriptor(DirectionalLight* owner) {
				Reference<ViewportLightDescriptor> descriptor =
					(owner->m_tmpLightDescriptor == nullptr) ? nullptr :
					dynamic_cast<ViewportLightDescriptor*>(owner->m_tmpLightDescriptor->Item());
				const Reference<const ViewportDescriptor> cameraViewport = 
					(owner->m_camera != nullptr) ? owner->m_camera->ViewportDescriptor() : nullptr;
				if (descriptor != nullptr && descriptor->Viewport() == cameraViewport) return;
				RemoveTemporaryLightDescriptor(owner);
				descriptor = Object::Instantiate<ViewportLightDescriptor>(this, cameraViewport);
				owner->m_tmpLightDescriptor = Object::Instantiate<LightDescriptor::Set::ItemOwner>(descriptor);
				owner->m_allLights->Add(owner->m_tmpLightDescriptor);
			}

			inline void UpdateTransform(DirectionalLight* owner) {
				LightSourceState::TransformState& state = m_lightState.transform;
				const Transform* transform = owner->GetTransfrom();
				if (transform == nullptr) state.rotation = Math::Identity();
				else state.rotation = transform->WorldRotationMatrix();
			}

			inline void UpdateColor(DirectionalLight* owner) {
				LightSourceState::ColorState& state = m_lightState.color;
				state.baseColor = owner->Color() * owner->Intensity();
				Graphics::TextureSampler* texture = owner->Texture();
				if (texture == nullptr)
					texture = m_whiteTexture->BoundObject();
				if (state.texture == nullptr || state.texture->BoundObject() != texture)
					state.texture = owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(texture);
				state.textureTiling = owner->TextureTiling();
				state.textureOffset = owner->TextureOffset();
			}

			inline void UpdateShadowInfo(DirectionalLight* owner) {
				LightSourceState::ShadowState& state = m_lightState.shadows;
				state.outOfFrustrumRange = owner->m_shadowRange;
				state.depthEpsilon = owner->m_depthEpsilon;
				state.resolution = owner->ShadowResolution();
				state.ambientAmount = owner->AmbientLightAmount();
				state.softness = owner->ShadowSoftness();
				state.kernelSize = owner->ShadowFilterSize();
				state.shadowSizeMultiplier =
					static_cast<float>(state.resolution + state.kernelSize + 1) /
					static_cast<float>(Math::Max(state.resolution, 1u));
				state.cascades = owner->m_cascades;
			}

		public:
			inline DirectionalLightInfo(DirectionalLight* owner, uint32_t typeId)
				: m_whiteTexture(Graphics::ShaderClass::SharedTextureSamplerBinding(Vector4(1.0f), owner->Context()->Graphics()->Device()))
				, m_typeId(typeId)
				, m_owner(owner) {
				m_owner->Context()->Graphics()->SynchPointJobs().Add(this);
			}

			inline virtual ~DirectionalLightInfo() {
				Dispose();
			}

			inline void Dispose() {
				Reference<DirectionalLight> owner = Owner();
				if (owner != nullptr) {
					RemoveTemporaryLightDescriptor(owner);
					m_owner->Context()->Graphics()->SynchPointJobs().Remove(this);
					std::unique_lock<SpinLock> lock(m_ownerLock);
					m_owner = nullptr;
				}
			}

			inline Reference<DirectionalLight> Owner()const {
				std::unique_lock<SpinLock> lock(m_ownerLock);
				Reference<DirectionalLight> owner = m_owner;
				return owner;
			}

			inline uint32_t TypeId()const { return m_typeId; }

			inline Event<DirectionalLight*>& OnUpdate() { return m_onUpdate; }

			inline const LightSourceState& State()const { return m_lightState; }

		protected:
			inline virtual void Execute()override {
				Reference<DirectionalLight> owner = Owner();
				if (owner == nullptr) return;
				UpdateTemporaryLightDescriptor(owner);
				UpdateTransform(owner);
				UpdateColor(owner);
				UpdateShadowInfo(owner);
				m_onUpdate(owner);
			}

			inline virtual void CollectDependencies(Callback<Job*>)override {}
		};

		inline static constexpr float ClosePlane() { return 0.1f; }
		inline static constexpr float ShadowMaxDepthDelta() { return 1.0f; }

		struct CameraFrustrum {
			struct Corner {
				Vector3 origin;
				Vector3 direction;
			};
			Corner a, b, c, d;

			inline CameraFrustrum(const ViewportDescriptor* viewport) {
				const Matrix4 viewMatrix = (viewport != nullptr) ? viewport->ViewMatrix() : Math::Identity();
				const Matrix4 projectionMatrix = (viewport != nullptr) ? viewport->ProjectionMatrix() : Math::Identity();

				const Matrix4 inversePoseMatrix = Math::Inverse(viewMatrix);
				const Matrix4 inverseCameraProjection = Math::Inverse(projectionMatrix);

				auto cameraClipToLocalSpace = [&](float x, float y, float z) -> Vector3 {
					Vector4 clipPos = inverseCameraProjection * Vector4(x, y, z, 1.0f);
					return (clipPos / clipPos.w);
				};

				auto getCorner = [&](float x, float y) {
					const Vector3 start = cameraClipToLocalSpace(x, y, 0.0f);
					const Vector3 end = cameraClipToLocalSpace(x, y, 1.0f);
					const Vector3 delta = (end - start);
					const Vector3 direction = delta / Math::Max(std::abs(delta.z), std::numeric_limits<float>::epsilon());
					return Corner{
						Vector3(inversePoseMatrix * Vector4(start - (start.z * direction), 1.0f)),
						Vector3(inversePoseMatrix * Vector4(direction, 0.0f)) };
				};

				a = getCorner(-1.0f, -1.0f);
				b = getCorner(-1.0f, 1.0f);
				c = getCorner(1.0f, 1.0f);
				d = getCorner(1.0f, -1.0f);
			}

			inline AABB GetRelativeBounds(float start, float end, const Matrix4& lightRotation)const {
				auto interpolatePoint = [](const Corner& corner, float phase) { 
					return corner.origin + (phase * corner.direction);
				};
				
				const Vector3 right = lightRotation[0];
				const Vector3 up = lightRotation[1];
				const Vector3 forward = lightRotation[2];
				auto relativePosition = [&](const Vector3& pos) {
					return Vector3(Math::Dot(pos, right), Math::Dot(pos, up), Math::Dot(pos, forward));
				};

				AABB bounds;
				bounds.start = bounds.end = relativePosition(interpolatePoint(a, start));
				
				auto includePoint = [&](const Corner& corner, float phase) {
					const Vector3 relPos = relativePosition(interpolatePoint(corner, phase));

					if (relPos.x < bounds.start.x) bounds.start.x = relPos.x;
					else if (relPos.x > bounds.end.x) bounds.end.x = relPos.x;

					if (relPos.y < bounds.start.y) bounds.start.y = relPos.y;
					else if (relPos.y > bounds.end.y) bounds.end.y = relPos.y;

					if (relPos.z < bounds.start.z) bounds.start.z = relPos.z;
					else if (relPos.z > bounds.end.z) bounds.end.z = relPos.z;
				};

				auto includeCorner = [&](const Corner& corner) {
					includePoint(corner, start);
					includePoint(corner, end);
				};

				includePoint(a, end);
				includeCorner(b);
				includeCorner(c);
				includeCorner(d);

				return bounds;
			}
		};

		struct DirectionalLightViewport : public virtual ViewportDescriptor {
			Matrix4 viewMatrix = Math::Identity();
			Matrix4 projectionMatrix = Math::Identity();
			float adjustedFarPlane = 0.0f;

			inline DirectionalLightViewport(Scene::LogicContext* context) : ViewportDescriptor(context) { }
			inline virtual ~DirectionalLightViewport() {}

			inline virtual Matrix4 ViewMatrix()const override { return viewMatrix; }
			inline virtual Matrix4 ProjectionMatrix()const override { return projectionMatrix; }
			inline virtual Vector4 ClearColor()const override { return Vector4(0.0f); }

			inline void Update(
				const CameraFrustrum& frustrum, const Matrix4& lightRotation, 
				float regionStart, float regionEnd, float farPlane, float shadowmapSizeMultiplier) {
				
				const AABB bounds = frustrum.GetRelativeBounds(regionStart, regionEnd, lightRotation);
				const float sizeX = bounds.end.x - bounds.start.x;
				const float sizeY = bounds.end.y - bounds.start.y;
				adjustedFarPlane = farPlane + (bounds.end.z - bounds.start.z);

				projectionMatrix = Math::Orthographic(Math::Max(sizeX, sizeY) * shadowmapSizeMultiplier, 1.0f, ClosePlane(), adjustedFarPlane);

				const Vector3 right = lightRotation[0];
				const Vector3 up = lightRotation[1];
				const Vector3 forward = lightRotation[2];
				auto toWorldSpace = [&](const Vector3& pos) {
					return (pos.x * right) + (pos.y * up) + (pos.z * forward);
				};
				const Vector3 center = toWorldSpace(Vector3(
					(bounds.start.x + bounds.end.x) * 0.5f,
					(bounds.start.y + bounds.end.y) * 0.5f,
					bounds.end.z + ShadowMaxDepthDelta()));
				Matrix4 pose = lightRotation;
				pose[3] = Vector4(center - (forward * adjustedFarPlane), 1.0f);
				
				// Note: We need to be able to manually control farPlane
				viewMatrix = Math::Inverse(pose);
			}
		};

#pragma warning(disable: 4250)
		struct CascadeShadowMapper {
			Reference<DirectionalLightViewport> lightmapperViewport;
			Reference<DepthOnlyRenderer> depthRenderer;
			Reference<VarianceShadowMapper> varianceShadowMapper;

			inline static CascadeShadowMapper Make(Scene::LogicContext* context) {
				CascadeShadowMapper shadowMapper;
				shadowMapper.lightmapperViewport = Object::Instantiate<DirectionalLightViewport>(context);
				shadowMapper.depthRenderer = Object::Instantiate<DepthOnlyRenderer>(shadowMapper.lightmapperViewport, LayerMask::All());
				shadowMapper.varianceShadowMapper = Object::Instantiate<VarianceShadowMapper>(context);
				return shadowMapper;
			}

			inline void Execute(
				const Graphics::Pipeline::CommandBufferInfo& commandBufferInfo, 
				const CameraFrustrum& frustrum,
				const LightSourceState& sourceState, size_t cascadeIndex) {
				float regionStart = 0.0f;
				float regionEnd = 0.0f;
				float regionStartDelta = 0.0f;
				for (size_t i = 0; i <= cascadeIndex; i++) {
					regionStart = regionEnd - regionStartDelta;
					const auto& cascade = sourceState.shadows.cascades[i];
					regionStartDelta = cascade.blendSize;
					regionEnd += cascade.size;
				}
				lightmapperViewport->Update(
					frustrum, sourceState.transform.rotation, 
					regionStart, regionEnd,
					sourceState.shadows.outOfFrustrumRange, 
					sourceState.shadows.shadowSizeMultiplier);
				varianceShadowMapper->Configure(
					ClosePlane(), lightmapperViewport->adjustedFarPlane,
					sourceState.shadows.softness, sourceState.shadows.kernelSize, true);
				depthRenderer->Render(commandBufferInfo);
				varianceShadowMapper->GenerateVarianceMap(commandBufferInfo);
			}
		};
		
		struct CascadedShadowMapper : public virtual JobSystem::Job {
			const Reference<const DirectionalLightInfo> lightDescriptor;
			const Reference<const ViewportDescriptor> viewportDescriptor;
			Reference<const TransientImage> depthTexture;
			Reference<Graphics::TextureSampler> depthTextureSampler;
			Stacktor<CascadeShadowMapper> cascades;

			inline CascadedShadowMapper(const DirectionalLightInfo* lightInfo, const ViewportDescriptor* viewport)
				: lightDescriptor(lightInfo), viewportDescriptor(viewport) {}

			inline virtual ~CascadedShadowMapper() {}

			inline virtual void Execute() override {
				const LightSourceState& sourceState = lightDescriptor->State();
				const CameraFrustrum frustrum(viewportDescriptor);
				const Graphics::Pipeline::CommandBufferInfo commandBufferInfo = viewportDescriptor->Context()->Graphics()->GetWorkerThreadCommandBuffer();
				for (size_t i = 0; i < cascades.Size(); i++)
					cascades[i].Execute(commandBufferInfo, frustrum, sourceState, i);
			}
			inline virtual void CollectDependencies(Callback<Job*>) override {}
		};

		class ViewportLightDescriptor 
			: public virtual LightDescriptor
			, public virtual LightDescriptor::ViewportData
			, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<DirectionalLightInfo> m_lightDescriptor;
			const Reference<const ViewportDescriptor> m_viewport;

			Reference<CascadedShadowMapper> m_shadowMapper;
			Stacktor<Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding>, 4u> m_shadowTextures;
			
			struct Data {
				Jimara_DirectionalLight_Data lightBuffer;
				std::atomic<bool> bufferDirty = true;
				SpinLock bufferLock;
				LightInfo lightInfo = {};
			};
			mutable Data m_data;

			inline void RemoveShadowMapper() {
				if (m_shadowMapper != nullptr) {
					m_viewport->Context()->Graphics()->RenderJobs().Remove(m_shadowMapper);
					m_shadowMapper = nullptr;
				}
				m_shadowTextures.Clear();
			}

			inline void UpdateShadowMapper(DirectionalLight* owner) {
				const LightSourceState& state = m_lightDescriptor->State();
				bool shadowMapperNeeded = (m_viewport != nullptr && owner->ShadowResolution() > 0);
				if (shadowMapperNeeded) {
					if (m_shadowMapper == nullptr) {
						m_shadowMapper = Object::Instantiate<CascadedShadowMapper>(m_lightDescriptor, m_viewport);
						m_viewport->Context()->Graphics()->RenderJobs().Add(m_shadowMapper);
					}

					const Size3 textureSize = Size3(owner->ShadowResolution(), owner->ShadowResolution(), 1u);
					const bool transientImageAbscent = !(
						m_shadowMapper->depthTexture != nullptr &&
						m_shadowMapper->depthTexture->Texture()->Size() == textureSize);
					if (transientImageAbscent) {
						m_shadowMapper->depthTexture = TransientImage::Get(
							owner->Context()->Graphics()->Device(),
							Graphics::Texture::TextureType::TEXTURE_2D,
							owner->Context()->Graphics()->Device()->GetDepthFormat(),
							textureSize, 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
						const Reference<Graphics::TextureView> depthTextureView =
							m_shadowMapper->depthTexture->Texture()->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
						m_shadowMapper->depthTextureSampler = depthTextureView->CreateSampler();
					}

					for (size_t i = 0; i < state.shadows.cascades.Size(); i++) {
						const bool cascadeAbscent = (m_shadowMapper->cascades.Size() <= i);
						if (cascadeAbscent)
							m_shadowMapper->cascades.Push(CascadeShadowMapper::Make(owner->Context()));

						const bool textureAbscent = (m_shadowTextures.Size() <= i);
						if (textureAbscent)
							m_shadowTextures.Push(nullptr);

						const bool recreateShadowMap = (transientImageAbscent || cascadeAbscent || textureAbscent);
						if (recreateShadowMap || m_shadowTextures[i] == nullptr) {
							CascadeShadowMapper& cascade = m_shadowMapper->cascades[i];
							cascade.depthRenderer->SetTargetTexture(m_shadowMapper->depthTextureSampler->TargetView());
							const Reference<Graphics::TextureSampler> varianceSampler = 
								cascade.varianceShadowMapper->SetDepthTexture(m_shadowMapper->depthTextureSampler, true);
							m_shadowTextures[i] = owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(varianceSampler);
						}
					}
					m_shadowMapper->cascades.Resize(state.shadows.cascades.Size());
					m_shadowTextures.Resize(state.shadows.cascades.Size());
				}
				else RemoveShadowMapper();
			}

			void UpdateData(DirectionalLight*) {
				const LightSourceState& state = m_lightDescriptor->State();
				Jimara_DirectionalLight_Data& buffer = m_data.lightBuffer;
				buffer.up = state.transform.rotation[1];
				buffer.forward = state.transform.rotation[2];
				const uint32_t numCascades = (m_shadowMapper == nullptr) ? 0u : static_cast<uint32_t>(
					Math::Min(m_shadowTextures.Size(), sizeof(buffer.cascades) / sizeof(Jimara_DirectionalLight_CascadeInfo)));
				buffer.numCascadesAndAmbientAmount = glm::packHalf2x16(Vector2(static_cast<float>(numCascades) + 0.5f, state.shadows.ambientAmount));
				// Set on request: buffer.viewportForward;
				buffer.color = state.color.baseColor;
				buffer.colorTextureId = state.color.texture->Index();
				buffer.textureTiling = glm::packHalf2x16(state.color.textureTiling);
				buffer.textureOffset = glm::packHalf2x16(Vector2(
					Math::FloatRemainder(state.color.textureOffset.x, 1.0f),
					Math::FloatRemainder(state.color.textureOffset.y, 1.0f)));
				for (uint32_t i = 0; i < numCascades; i++) {
					Jimara_DirectionalLight_CascadeInfo& cascade = buffer.cascades[i];
					const ShadowCascadeInfo& info = state.shadows.cascades[i];
					// Set on request: cascade.lightmapOffset;
					// Set on request: cascade.lightmapSize;
					// Set on request: cascade.lightmapDepth;
					// Set on request: cascade.inverseFarPlane;
					cascade.viewportDistance = info.size + ((i > 0) ? buffer.cascades[i - 1].viewportDistance : 0.0f);
					cascade.blendDistance = info.blendSize;
					cascade.shadowSamplerId = m_shadowTextures[i]->Index();
				}
				m_data.bufferDirty = true;
			}

			void Update(DirectionalLight* owner) {
				if (owner == nullptr) return;
				UpdateShadowMapper(owner);
				UpdateData(owner);
			}

		public:
			inline ViewportLightDescriptor(DirectionalLightInfo* descriptor, const ViewportDescriptor* viewport)
				: m_lightDescriptor(descriptor), m_viewport(viewport) {
				m_data.lightInfo.typeId = descriptor->TypeId();
				m_data.lightInfo.data = &m_data.lightBuffer;
				m_data.lightInfo.dataSize = sizeof(Jimara_DirectionalLight_Data);
				m_lightDescriptor->OnUpdate() += Callback(&ViewportLightDescriptor::Update, this);
			}

			inline virtual ~ViewportLightDescriptor() {
				RemoveShadowMapper();
				m_lightDescriptor->OnUpdate() -= Callback(&ViewportLightDescriptor::Update, this);
			}

			inline const ViewportDescriptor* Viewport()const { return m_viewport; }

			virtual Reference<const LightDescriptor::ViewportData> GetViewportData(const ViewportDescriptor*)override { return this; }
			inline virtual LightInfo GetLightInfo()const override {
				if (m_data.bufferDirty) {
					const CameraFrustrum frustrum(m_viewport);
					const Matrix4 poseMatrix = (m_viewport == nullptr) ? Math::Identity() : Math::Inverse(m_viewport->ViewMatrix());
					const LightSourceState& state = m_lightDescriptor->State();
					const Vector3 viewportForward = Math::Normalize(Vector3(poseMatrix[2]));
					const float viewportDelta = Math::Dot(viewportForward, Vector3(poseMatrix[3]));
					
					std::unique_lock<SpinLock> bufferLock(m_data.bufferLock);
					if (m_data.bufferDirty) {
						Jimara_DirectionalLight_Data& buffer = m_data.lightBuffer;
						buffer.viewportForward = viewportForward;
						float regionStart = 0.0f;
						float regionEnd = 0.0f;
						float regionStartDelta = 0.0f;
						const uint32_t numCascades = static_cast<uint32_t>(glm::unpackHalf2x16(buffer.numCascadesAndAmbientAmount).x);
						for (uint32_t i = 0; i < numCascades; i++) {
							Jimara_DirectionalLight_CascadeInfo& cascade = buffer.cascades[i];
							regionStart = regionEnd - regionStartDelta;
							regionStartDelta = cascade.blendDistance;
							regionEnd = cascade.viewportDistance;

							const AABB relativeBounds = frustrum.GetRelativeBounds(regionStart, regionEnd, state.transform.rotation);
							const float lightmapSize = Math::Max(Math::Max(
								relativeBounds.end.x - relativeBounds.start.x,
								relativeBounds.end.y - relativeBounds.start.y), std::numeric_limits<float>::epsilon()) * state.shadows.shadowSizeMultiplier;
							const Vector3 center = (relativeBounds.start + relativeBounds.end) * 0.5f;
							float adjustedFarPlane = state.shadows.outOfFrustrumRange + (relativeBounds.end.z - relativeBounds.start.z);

							cascade.lightmapSize = 1.0f / lightmapSize;
							cascade.lightmapOffset = Vector2(center.x * -cascade.lightmapSize, center.y * cascade.lightmapSize) + 0.5f;
							cascade.lightmapDepth = -(relativeBounds.end.z + ShadowMaxDepthDelta() + adjustedFarPlane * (state.shadows.depthEpsilon - 1.0f));
							cascade.inverseFarPlane = 1.0f / Math::Max(adjustedFarPlane, std::numeric_limits<float>::epsilon());
							cascade.viewportDistance += viewportDelta;
						}
						m_data.bufferDirty = false;
					}
				}
				return m_data.lightInfo;
			}
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
		};
#pragma warning(default: 4250)

		class CascadeListSerializer : public virtual Serialization::SerializerList::From<Stacktor<ShadowCascadeInfo, 4u>> {
		public:
			inline CascadeListSerializer(const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes = {})
				: Serialization::ItemSerializer(name, hint, attributes) {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Stacktor<ShadowCascadeInfo, 4u>* target)const override {
				JIMARA_SERIALIZE_FIELDS(&target, recordElement) {
					{
						size_t count = target->Size();
						JIMARA_SERIALIZE_FIELD(count, "Count", "Cascade count", Object::Instantiate<Serialization::SliderAttribute<size_t>>(1u, 4u));
						count = Math::Min(Math::Max((size_t)1u, count), (size_t)4u);
						if (target->Size() != count) target->Resize(count);
					}
					for (size_t i = 0u; i < target->Size(); i++)
						JIMARA_SERIALIZE_FIELD(target->operator[](i), "Cascade", "Cascade parameters");
				};
			}
		};
	};

	void DirectionalLight::ShadowCascadeInfo::Serializer::GetFields(const Callback<Serialization::SerializedObject>& recordElement, ShadowCascadeInfo* target)const {
		JIMARA_SERIALIZE_FIELDS(&target, recordElement) {
			JIMARA_SERIALIZE_FIELD(target->size, "Size", "Cascade size in units");
			if (target->size < 0.0f) target->size = 0.0f;
			JIMARA_SERIALIZE_FIELD(target->blendSize, "Blend Size", "Size that should be blended with the next cascade");
			target->blendSize = Math::Min(Math::Max(0.0f, target->blendSize), target->size);
		};
	}

	DirectionalLight::DirectionalLight(Component* parent, const std::string_view& name, Vector3 color)
		: Component(parent, name)
		, m_allLights(LightDescriptor::Set::GetInstance(parent->Context()))
		, m_color(color) {}

	DirectionalLight::~DirectionalLight() {
		OnComponentDisabled();
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<DirectionalLight>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<DirectionalLight> serializer("Jimara/Lights/DirectionalLight", "Directional light component");
		report(&serializer);
	}


	Vector3 DirectionalLight::Color()const { return m_color; }

	void DirectionalLight::SetColor(Vector3 color) { m_color = color; }

	void DirectionalLight::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Color, SetColor, "Color", "Light color", Object::Instantiate<Serialization::ColorAttribute>());
			JIMARA_SERIALIZE_FIELD_GET_SET(Intensity, SetIntensity, "Intensity", "Color multiplier");
			JIMARA_SERIALIZE_FIELD_GET_SET(Texture, SetTexture, "Texture", "Optionally, the spotlight projection can take color form this texture");
			if (Texture() != nullptr) {
				JIMARA_SERIALIZE_FIELD_GET_SET(TextureTiling, SetTextureTiling, "Tiling", "Tells, how many times should the texture repeat itself");
				JIMARA_SERIALIZE_FIELD_GET_SET(TextureOffset, SetTextureOffset, "Offset", "Tells, how to shift the texture around");
			}
			JIMARA_SERIALIZE_FIELD(m_camera, "Camera", "[Temporary] camera reference");
			JIMARA_SERIALIZE_FIELD(m_shadowRange, "Shadow Range", "[Temporary] Shadow renderer far plane");
			JIMARA_SERIALIZE_FIELD(m_depthEpsilon, "Shadow Depth Epsilon", "[Temporary] Minimal shadow distance");
			JIMARA_SERIALIZE_FIELD_GET_SET(ShadowResolution, SetShadowResolution, "Shadow Resolution", "Resolution of the shadow",
				Object::Instantiate<Serialization::EnumAttribute<uint32_t>>(false,
					"No Shadows", 0u,
					"32 X 32", 32u,
					"64 X 64", 64u,
					"128 X 128", 128u,
					"256 X 256", 256u,
					"512 X 512", 512u,
					"1024 X 1024", 1024u,
					"2048 X 2048", 2048u,
					"4096 X 4096", 4096u));
			if (ShadowResolution() > 0u) {
				JIMARA_SERIALIZE_FIELD_GET_SET(AmbientLightAmount, SetAmbientLightAmount, "Ambient Light Amount", "Fraction of the light, still present in the shadowed areas",
					Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
				JIMARA_SERIALIZE_FIELD_GET_SET(ShadowSoftness, SetShadowSoftness, "Shadow Softness", "Tells, how soft the cast shadow is",
					Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
				JIMARA_SERIALIZE_FIELD_GET_SET(ShadowFilterSize, SetShadowFilterSize, "Filter Size", "Tells, what size kernel is used for rendering soft shadows",
					Object::Instantiate<Serialization::SliderAttribute<uint32_t>>(1u, 65u, 2u));
				{
					static const Helpers::CascadeListSerializer serializer("Cascades", "Cascade definitions");
					recordElement(serializer.Serialize(m_cascades));
				}
			}
		};
	}

	void DirectionalLight::OnComponentEnabled() {
		if (!ActiveInHeirarchy())
			OnComponentDisabled();
		else if (m_synchJob == nullptr) {
			uint32_t typeId;
			if (Context()->Graphics()->Configuration().ShaderLoader()->GetLightTypeId("Jimara_DirectionalLight", typeId))
				m_synchJob = Object::Instantiate<Helpers::DirectionalLightInfo>(this, typeId);
		}
	}

	void DirectionalLight::OnComponentDisabled() {
		if (ActiveInHeirarchy())
			OnComponentEnabled();
		else if (m_synchJob != nullptr) {
			dynamic_cast<Helpers::DirectionalLightInfo*>(m_synchJob.operator->())->Dispose();
			m_synchJob = nullptr;
		}
	}
}
