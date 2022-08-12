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
		struct Jimara_DirectionalLight_CascadeInfo {
			alignas(8) Vector2 lightmapOffset;		// Bytes [0 - 8)	Lightmap UV offset (center (X, Y coordinates) * lightmapSize + 0.5f) in "light space"
			alignas(4) float lightmapSize;			// Bytes [8 - 12)	Lightmap orthographic size
			alignas(4) float lightmapDepth;			// Bytes [12 - 16)	Inversed Z coordinate of the lightmap's view matrix in "light space"
			alignas(4) float viewportDistance;		// Bytes [16 - 20)	Maximal distance from viewport, this lightmap will cover
			alignas(4) float blendDistance;			// Bytes [20 - 24)	Blended region size between this cascade and the next one (fade size for the last cascade)
			alignas(4) uint32_t shadowSamplerId;	// Bytes [24 - 28)	Sampler index in the global bindless array
													// Bytes [28 - 32)	Padding
		};
		static_assert(sizeof(Jimara_DirectionalLight_CascadeInfo) == 32);

		struct Jimara_DirectionalLight_Data {
			alignas(16) Vector3 up;					// Bytes [0 - 12)	lightRotation.up
			alignas(16) Vector3 forward;			// Bytes [16 - 28)	lightRotation.forward
			alignas(4) uint32_t numCascades;		// Bytes [28 - 32)	Number of used shadow cascades
			alignas(16) Vector3 viewportForward;	// Bytes [32 - 44)	viewMatrix.forward
			alignas(4) float viewportOrigin;		// Bytes [44 - 48)	-dot(viewMatrix.position, viewportForward)
			alignas(16) Vector3 color;				// Bytes [48 - 60)	Color() * Intensity()

			Jimara_DirectionalLight_CascadeInfo cascades[4];	// Bytes [64 - 192)
		};
		static_assert(sizeof(Jimara_DirectionalLight_Data) == 192);

		struct LightData {
			alignas(16) Vector3 up;					// Bytes [0 - 12)
			alignas(16) Vector3 forward;			// Bytes [16 - 28)

			alignas(4) uint32_t shadowSamplerId;	// Bytes [28, 32)
			alignas(8) Vector2 lightmapOffset;		// Bytes [32, 40)
			alignas(4) float lightmapSize;			// Bytes [40, 44)
			alignas(4) float lightmapDepth;			// Bytes [44, 48)

			alignas(16) Vector3 color;				// Bytes [48, 60)
			alignas(4) float lightmapInvRange;		// Bytes [60, 64)
		};

		static_assert(sizeof(LightData) == 64);

		inline static constexpr float ClosePlane() { return 0.1f; }
		inline static constexpr float ShadowMaxDepthDelta() { return 1.0f; }
		inline static constexpr float FirstCascadeRange() { return 1.0f; }

		class DirectionalLightDescriptor;

		struct CameraFrustrum {
			struct Corner {
				Vector3 start = Vector3(0.0f);
				Vector3 end = Vector3(0.0f);
			};
			Corner a, b, c, d;

			inline CameraFrustrum(const ViewportDescriptor* viewport) {
				const Matrix4 viewMatrix = (viewport != nullptr) ? viewport->ViewMatrix() : Math::Identity();
				const Matrix4 projectionMatrix = (viewport != nullptr) ? viewport->ProjectionMatrix() : Math::Identity();

				const Matrix4 inversePoseMatrix = Math::Inverse(viewMatrix);
				const Matrix4 inverseCameraProjection = Math::Inverse(projectionMatrix);

				auto cameraClipToWorldSpace = [&](float x, float y, float z) -> Vector3 {
					Vector4 clipPos = inverseCameraProjection * Vector4(x, y, z, 1.0f);
					return inversePoseMatrix * (clipPos / clipPos.w);
				};

				auto getCorner = [&](float x, float y) {
					return Corner{ cameraClipToWorldSpace(x, y, 0.0f), cameraClipToWorldSpace(x, y, 1.0f) };
				};

				a = getCorner(-1.0f, -1.0f);
				b = getCorner(-1.0f, 1.0f);
				c = getCorner(1.0f, 1.0f);
				d = getCorner(1.0f, -1.0f);
			}

			inline AABB GetRelativeBounds(float start, float end, const Matrix4& lightRotation)const {
				auto interpolatePoint = [](const Corner& corner, float phase) { return Math::Lerp(corner.start, corner.end, phase); };
				
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
		struct ShadowMapper : public virtual JobSystem::Job, public virtual ObjectCache<Reference<const Object>>::StoredObject {
			const Reference<DirectionalLightViewport> lightmapperViewport;
			const Reference<const ViewportDescriptor> cameraViewport;
			const Reference<DirectionalLightDescriptor> descriptor;
			const Reference<DepthOnlyRenderer> depthRenderer;
			const Reference<VarianceShadowMapper> varianceShadowMapper;

			inline ShadowMapper(DirectionalLightViewport* viewport, const ViewportDescriptor* cameraView, DirectionalLightDescriptor* desc)
				: lightmapperViewport(viewport)
				, cameraViewport(cameraView)
				, descriptor(desc)
				, depthRenderer(Object::Instantiate<DepthOnlyRenderer>(viewport, LayerMask::All()))
				, varianceShadowMapper(Object::Instantiate<VarianceShadowMapper>(viewport->Context())) {}

			inline virtual ~ShadowMapper() {}

			inline virtual void Execute() override {
				const Graphics::Pipeline::CommandBufferInfo commandBufferInfo = lightmapperViewport->Context()->Graphics()->GetWorkerThreadCommandBuffer();
				const CameraFrustrum frustrum(cameraViewport);
				lightmapperViewport->Update(frustrum, descriptor->m_rotation, 0.0f, FirstCascadeRange(), descriptor->m_shadowFarPlane, descriptor->m_shadowmapSizeMultiplier);
				varianceShadowMapper->Configure(ClosePlane(), lightmapperViewport->adjustedFarPlane, descriptor->m_shadowSoftness, descriptor->m_vsmKernelSize, true);
				depthRenderer->Render(commandBufferInfo);
				varianceShadowMapper->GenerateVarianceMap(commandBufferInfo);
			}
			inline virtual void CollectDependencies(Callback<Job*>) override {}
		};
#pragma warning(default: 4250)

		class DirectionalLightDescriptor 
			: public virtual LightDescriptor
			, public virtual JobSystem::Job {
		public:
			DirectionalLight* m_owner;

		private:
			friend struct ShadowMapper;

			const Reference<const Graphics::ShaderClass::TextureSamplerBinding> m_whiteTexture;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_shadowTexture;
			Reference<const TransientImage> m_depthTexture;
			float m_shadowFarPlane = 100.0f;
			float m_shadowmapSizeMultiplier = 1.0f;
			float m_shadowDepthEpsilon = 0.1f;
			float m_shadowSoftness = 0.5f;
			uint32_t m_vsmKernelSize = 1;

			mutable LightData m_data;
			Matrix4 m_rotation = Math::Identity();
			mutable std::atomic<bool> m_dataDirty = true;
			mutable SpinLock m_dataLock;

			LightInfo m_info;

			inline void UpdateShadowRenderer() {
				if (m_owner->m_shadowResolution <= 0u) {
					if (m_owner->m_shadowRenderJob != nullptr) {
						m_owner->Context()->Graphics()->RenderJobs().Remove(m_owner->m_shadowRenderJob);
						m_owner->m_shadowRenderJob = nullptr;
					}
					m_owner->m_shadowTexture = nullptr;
					m_depthTexture = nullptr;
				}
				else {
					Reference<ShadowMapper> shadowMapper = m_owner->m_shadowRenderJob;
					if (shadowMapper == nullptr) {
						Reference<DirectionalLightViewport> viewport = 
							Object::Instantiate<DirectionalLightViewport>(m_owner->Context());
						const ViewportDescriptor* cameraView = (m_owner->m_camera == nullptr) ? nullptr : m_owner->m_camera->ViewportDescriptor();
						shadowMapper = Object::Instantiate<ShadowMapper>(viewport, cameraView, this);
						m_owner->m_shadowRenderJob = shadowMapper;
						m_owner->m_shadowTexture = nullptr;
						m_owner->Context()->Graphics()->RenderJobs().Add(m_owner->m_shadowRenderJob);
						m_depthTexture = nullptr;
					}

					const Size3 textureSize = Size3(m_owner->m_shadowResolution, m_owner->m_shadowResolution, 1u);
					if (m_owner->m_shadowTexture == nullptr ||
						m_owner->m_shadowTexture->TargetView()->TargetTexture()->Size() != textureSize) {
						m_depthTexture = TransientImage::Get(m_owner->Context()->Graphics()->Device(),
							Graphics::Texture::TextureType::TEXTURE_2D, shadowMapper->depthRenderer->TargetTextureFormat(),
							textureSize, 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
						const auto view = m_depthTexture->Texture()->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
						const auto sampler = view->CreateSampler(
							Graphics::TextureSampler::FilteringMode::LINEAR,
							Graphics::TextureSampler::WrappingMode::REPEAT);
						shadowMapper->depthRenderer->SetTargetTexture(view);
						m_owner->m_shadowTexture = shadowMapper->varianceShadowMapper->SetDepthTexture(sampler, true);
					}
				}

				// Shadow settings:
				{
					m_shadowFarPlane = m_owner->m_shadowRange + ClosePlane() + ShadowMaxDepthDelta();
					m_shadowDepthEpsilon = m_owner->m_depthEpsilon;
					m_shadowSoftness = m_owner->ShadowSoftness();
					m_vsmKernelSize = m_owner->ShadowFilterSize();
					m_shadowmapSizeMultiplier =
						static_cast<float>(m_owner->ShadowResolution() + m_vsmKernelSize + 1) /
						static_cast<float>(Math::Max(m_owner->ShadowResolution(), 1u));
				}
			}

			inline void UpdateData() {
				if (m_owner == nullptr) return;
				
				m_rotation = [&]() {
					const Transform* transform = m_owner->GetTransfrom();
					if (transform == nullptr) return Math::Identity();
					else return transform->WorldRotationMatrix();
				}();

				// Pose:
				{
					m_data.up = m_rotation[1];
					m_data.forward = m_rotation[2];
				}

				// Shadow texture:
				{
					if (m_shadowTexture == nullptr || m_shadowTexture->BoundObject() != m_owner->m_shadowTexture) {
						if (m_owner->m_shadowTexture == nullptr) {
							if (m_shadowTexture == nullptr || m_shadowTexture->BoundObject() != m_whiteTexture->BoundObject())
								m_shadowTexture = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(m_whiteTexture->BoundObject());
						}
						else m_shadowTexture = m_owner->Context()->Graphics()->Bindless().Samplers()->GetBinding(m_owner->m_shadowTexture);
					}
					m_data.shadowSamplerId = m_shadowTexture->Index();
				}

				// Note: lightmapOffset, lightmapSize and lightmapDepth should be filled-in after camera viewport is updated

				// Color & range:
				{
					m_data.color = m_owner->Color();
					m_data.lightmapInvRange =
						(m_owner->ShadowResolution() <= 0u) ? 0.0f :
						1.0f / Math::Max(m_shadowFarPlane, std::numeric_limits<float>::epsilon());
				}

				m_dataDirty = true;
			}

		public:
			inline DirectionalLightDescriptor(DirectionalLight* owner, uint32_t typeId) 
				: m_owner(owner)
				, m_whiteTexture(Graphics::ShaderClass::SharedTextureSamplerBinding(Vector4(1.0f), owner->Context()->Graphics()->Device()))
				, m_info{} {
				UpdateData();
				m_info.typeId = typeId;
				m_info.data = &m_data;
				m_info.dataSize = sizeof(LightData);
			}

			// LightDescriptor:
			virtual LightInfo GetLightInfo()const override { 
				if (!m_dataDirty) return m_info;
				
				const CameraFrustrum frustrum((m_owner == nullptr) ? nullptr : (m_owner->m_camera == nullptr) ? nullptr : m_owner->m_camera->ViewportDescriptor());
				const AABB relativeBounds = frustrum.GetRelativeBounds(0.0f, FirstCascadeRange(), m_rotation);
				const float lightmapSize = Math::Max(Math::Max(
					relativeBounds.end.x - relativeBounds.start.x,
					relativeBounds.end.y - relativeBounds.start.y), std::numeric_limits<float>::epsilon()) * m_shadowmapSizeMultiplier;
				const Vector3 center = (relativeBounds.start + relativeBounds.end) * 0.5f;
				float adjustedFarPlane = m_shadowFarPlane + (relativeBounds.end.z - relativeBounds.start.z);
				{
					std::unique_lock<SpinLock> lock(m_dataLock);
					if (!m_dataDirty) return m_info;
					m_data.lightmapSize = 1.0f / lightmapSize;
					m_data.lightmapOffset = Vector2(center.x * -m_data.lightmapSize, center.y * m_data.lightmapSize) + 0.5f;
					m_data.lightmapDepth = relativeBounds.end.z + ShadowMaxDepthDelta() - adjustedFarPlane + (m_shadowDepthEpsilon * adjustedFarPlane);
					m_dataDirty = false;
				}

				return m_info; 
			}

			virtual AABB GetLightBounds()const override {
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
				UpdateShadowRenderer();
				UpdateData(); 
			}
			virtual void CollectDependencies(Callback<Job*>)override {}
		};
	};

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
			JIMARA_SERIALIZE_FIELD(m_camera, "Camera", "[Temporary] camera reference");
			JIMARA_SERIALIZE_FIELD(m_shadowRange, "Shadow Range", "[Temporary] Shadow renderer far plane");
			JIMARA_SERIALIZE_FIELD(m_depthEpsilon, "Shadow Range", "[Temporary] Minimal shadow distance");
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
				JIMARA_SERIALIZE_FIELD_GET_SET(ShadowSoftness, SetShadowSoftness, "Shadow Softness", "Tells, how soft the cast shadow is",
					Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
				JIMARA_SERIALIZE_FIELD_GET_SET(ShadowFilterSize, SetShadowFilterSize, "Filter Size", "Tells, what size kernel is used for rendering soft shadows",
					Object::Instantiate<Serialization::SliderAttribute<uint32_t>>(1u, 65u, 2u));
			}
		};
	}

	void DirectionalLight::OnComponentEnabled() {
		if (!ActiveInHeirarchy())
			OnComponentDisabled();
		else if (m_lightDescriptor == nullptr) {
			uint32_t typeId;
			if (Context()->Graphics()->Configuration().ShaderLoader()->GetLightTypeId("Jimara_DirectionalLight", typeId)) {
				Reference<Helpers::DirectionalLightDescriptor> descriptor = Object::Instantiate<Helpers::DirectionalLightDescriptor>(this, typeId);
				m_lightDescriptor = Object::Instantiate<LightDescriptor::Set::ItemOwner>(descriptor);
				m_allLights->Add(m_lightDescriptor);
				Context()->Graphics()->SynchPointJobs().Add(descriptor);
			}
		}
	}

	void DirectionalLight::OnComponentDisabled() {
		if (ActiveInHeirarchy())
			OnComponentEnabled();
		else {
			if (m_lightDescriptor != nullptr) {
				m_allLights->Remove(m_lightDescriptor);
				Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<JobSystem::Job*>(m_lightDescriptor->Item()));
				dynamic_cast<Helpers::DirectionalLightDescriptor*>(m_lightDescriptor->Item())->m_owner = nullptr;
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
