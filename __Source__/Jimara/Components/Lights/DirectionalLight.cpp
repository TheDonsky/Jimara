#include "DirectionalLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Environment/Rendering/LightingModels/DepthOnlyRenderer/DepthOnlyRenderer.h"
#include "../../Environment/Rendering/Shadows/VarianceShadowMapper/VarianceShadowMapper.h"
#include "../../Environment/Rendering/TransientImage.h"
#include <limits>


namespace Jimara {
	struct DirectionalLight::Helpers {
#define JIMARA_DIRECTIONAL_LIGHT_USE_SHADOWS

#ifndef JIMARA_DIRECTIONAL_LIGHT_USE_SHADOWS
		struct LightData {
			alignas(16) Vector3 direction;
			alignas(16) Vector3 color;
		};

		static_assert(sizeof(LightData) == 32);

#else
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

		inline static constexpr float ShadowRange() { return 1000.0f; }
#endif

		class DirectionalLightDescriptor;

		struct CameraFrustrum {
			struct Corner {
				Vector3 start = Vector3(0.0f);
				Vector3 end = Vector3(0.0f);
			};
			Corner a, b, c, d;

			inline CameraFrustrum(const ViewportDescriptor* viewport) {
				const Matrix4 viewMatrix = viewport->ViewMatrix();
				const Matrix4 projectionMatrix = viewport->ProjectionMatrix();

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
				
				AABB bounds;
				bounds.start = bounds.end = interpolatePoint(a, start);
				
				const Vector3 right = lightRotation[0];
				const Vector3 up = lightRotation[1];
				const Vector3 forward = lightRotation[2];
				
				auto includePoint = [&](const Corner& corner, float phase) {
					const Vector3 relPos = interpolatePoint(corner, phase);

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

			inline DirectionalLightViewport(Scene::LogicContext* context) : ViewportDescriptor(context) { }
			inline virtual ~DirectionalLightViewport() {}

			inline virtual Matrix4 ViewMatrix()const override { return viewMatrix; }
			inline virtual Matrix4 ProjectionMatrix()const override { return projectionMatrix; }
			inline virtual Vector4 ClearColor()const override { return Vector4(0.0f); }

			inline void Update(const CameraFrustrum& frustrum, const Matrix4& lightRotation, float regionStart, float regionEnd) {
				const constexpr float closePlane = 0.001f;
				const constexpr float farPlane = ShadowRange();
				const constexpr float zDelta = 10.0f;

				const AABB bounds = frustrum.GetRelativeBounds(regionStart, regionEnd, lightRotation);
				const float sizeX = bounds.end.x - bounds.start.x;
				const float sizeY = bounds.end.y - bounds.start.y;
				const float aspectRatio = (sizeX / Math::Max(sizeY, std::numeric_limits<float>::epsilon()));

				// Note: We need additional padding here for the gaussian blur...
				viewMatrix = Math::Orthographic(sizeY, aspectRatio, closePlane, farPlane);

				const Vector3 right = lightRotation[0];
				const Vector3 up = lightRotation[1];
				const Vector3 forward = lightRotation[2];
				auto toWorldSpace = [&](const Vector3& pos) {
					return (pos.x * right) + (pos.y * up) + (pos.z * forward);
				};
				const Vector3 center = toWorldSpace(Vector3(
					(bounds.start.x + bounds.end.x) * 0.5f,
					(bounds.start.y + bounds.end.y) * 0.5f,
					bounds.end.z + zDelta));
				Matrix4 pose = lightRotation;
				pose[3] = Vector4(center - (forward * farPlane), 1.0f);
				
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
			const Reference<VarianceShadowMapper> shadowMapper;

			inline ShadowMapper(DirectionalLightViewport* viewport, ViewportDescriptor* cameraView, DirectionalLightDescriptor* desc)
				: lightmapperViewport(viewport)
				, cameraViewport(cameraView)
				, descriptor(desc)
				, depthRenderer(Object::Instantiate<DepthOnlyRenderer>(viewport, LayerMask::All()))
				, shadowMapper(Object::Instantiate<VarianceShadowMapper>(viewport->Context())) {}

			inline virtual ~ShadowMapper() {}

			inline virtual void Execute() override {
				const Graphics::Pipeline::CommandBufferInfo commandBufferInfo = lightmapperViewport->Context()->Graphics()->GetWorkerThreadCommandBuffer();
				const CameraFrustrum frustrum(cameraViewport);
				lightmapperViewport->Update(frustrum, descriptor->m_rotation, 0.0f, 0.25f);
				depthRenderer->Render(commandBufferInfo);
				shadowMapper->GenerateVarianceMap(commandBufferInfo);
			}
			inline virtual void CollectDependencies(Callback<Job*>) override {}
		};
#pragma warning(default: 4250)

		class DirectionalLightDescriptor 
			: public virtual LightDescriptor
			, public virtual JobSystem::Job {
		public:
			const DirectionalLight* m_owner;

		private:
			friend struct ShadowMapper;

			const Reference<const Graphics::ShaderClass::TextureSamplerBinding> m_whiteTexture;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_shadowTexture;
			Reference<const TransientImage> m_depthTexture;

			LightData m_data;
			Matrix4 m_rotation = Math::Identity();

			LightInfo m_info;

			inline void UpdateShadowRenderer() {

			}

			inline void UpdateData() {
				if (m_owner == nullptr) return;
				
				m_rotation = [&]() {
					const Transform* transform = m_owner->GetTransfrom();
					if (transform == nullptr) return Math::Identity();
					else return transform->WorldRotationMatrix();
				}();

#ifndef JIMARA_DIRECTIONAL_LIGHT_USE_SHADOWS
				m_data.direction = m_rotation[2];
				m_data.color = m_owner->Color();


#else
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
					m_data.lightmapInvRange = 1.0f / ShadowRange();
				}
#endif

			}

		public:
			inline DirectionalLightDescriptor(const DirectionalLight* owner, uint32_t typeId) 
				: m_owner(owner)
				, m_whiteTexture(Graphics::ShaderClass::SharedTextureSamplerBinding(Vector4(1.0f), owner->Context()->Graphics()->Device()))
				, m_info{} {
				UpdateData();
				m_info.typeId = typeId;
				m_info.data = &m_data;
				m_info.dataSize = sizeof(LightData);
			}

			// LightDescriptor:
			virtual LightInfo GetLightInfo()const override { return m_info; }
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
