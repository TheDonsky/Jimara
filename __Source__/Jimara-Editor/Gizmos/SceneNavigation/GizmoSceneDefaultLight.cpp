#include "GizmoSceneDefaultLight.h"
#include <Environment/Rendering/ImageBasedLighting/HDRIEnvironment.h>
#include <Math/Random.h>


namespace Jimara {
	namespace Editor {
		struct GizmoSceneDefaultLight::Helpers {
			struct LightData {
				alignas(16) Vector3 color = Vector3(1.0f);
				alignas(4) uint32_t irradianceID = 0u;
				alignas(4) uint32_t preFilteredMapID = 0u;
				alignas(4) uint32_t environmentMapID = 0u;
				alignas(4) uint32_t brdfIntegrationMapID = 0u;
				alignas(4) float preFilteredMapMipCount = 1.0f;
			};
			static_assert(sizeof(LightData) == 32u);

			class DefaultLightDescriptor
				: public virtual LightDescriptor
				, public virtual LightDescriptor::ViewportData {
			private:
				const Reference<GizmoScene::Context> m_gizmoContext;
				const Reference<const Graphics::ShaderClass::TextureSamplerBinding> m_whiteTexture;
				const LightData m_data;
				const LightInfo m_info;

				class DarkLightDescriptor : public virtual LightDescriptor::ViewportData {
				private:
					const Reference<const Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_whiteTextureBinding;
					const Reference<const Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_brdfIntegrationMapBinding;
					const LightData m_data;
					const LightInfo m_info;
					const Vector3 m_position;

				public:
					inline DarkLightDescriptor(
						uint32_t typeId,
						const Graphics::BindlessSet<Graphics::TextureSampler>::Binding* whiteTextureBinding,
						const Graphics::BindlessSet<Graphics::TextureSampler>::Binding* brdfIntegrationMapBinding) 
						: m_whiteTextureBinding(whiteTextureBinding)
						, m_brdfIntegrationMapBinding(brdfIntegrationMapBinding)
						, m_data([&]() {
							LightData data = {};
							data.color = Vector3(0.0f);
							data.irradianceID = data.preFilteredMapID = data.environmentMapID = whiteTextureBinding->Index();
							data.brdfIntegrationMapID = brdfIntegrationMapBinding->Index();
							return data;
						}())
						, m_info([&]() {
							LightInfo info = {};
							info.data = &m_data;
							info.dataSize = sizeof(LightData);
							info.typeId = typeId;
							return info;
						}())
						, m_position(Random::PointInSphere() * 1000.0f) {
						assert(m_whiteTextureBinding != nullptr);
						assert(m_brdfIntegrationMapBinding != nullptr);
					}

					inline virtual ~DarkLightDescriptor() {}

					inline virtual LightInfo GetLightInfo()const override { return m_info; }
					inline virtual AABB GetLightBounds()const override { return AABB(m_position, m_position); }
				};
				const Reference<const LightDescriptor::ViewportData> m_darkLightDescriptor;

			public:
				inline DefaultLightDescriptor(
					GizmoScene::Context* gizmoContext, uint32_t typeId,
					const Graphics::ShaderClass::TextureSamplerBinding* whiteTexture,
					const Graphics::BindlessSet<Graphics::TextureSampler>::Binding* whiteTextureBinding,
					const Graphics::BindlessSet<Graphics::TextureSampler>::Binding* brdfIntegrationMapBinding)
					: m_gizmoContext(gizmoContext)
					, m_whiteTexture(whiteTexture)
					, m_data([&]() {
						LightData data = {};
						data.irradianceID = data.preFilteredMapID = data.environmentMapID = whiteTextureBinding->Index();
						data.brdfIntegrationMapID = brdfIntegrationMapBinding->Index();
						return data;
					}())
					, m_info([&]() {
						LightInfo info = {};
						info.data = &m_data;
						info.dataSize = sizeof(LightData);
						info.typeId = typeId;
						return info;
					}())
					, m_darkLightDescriptor(Object::Instantiate<DarkLightDescriptor>(typeId, whiteTextureBinding, brdfIntegrationMapBinding)) {
					assert(m_gizmoContext != nullptr);
					assert(m_whiteTexture != nullptr);
				}
				inline virtual ~DefaultLightDescriptor() {}

				inline virtual Reference<const LightDescriptor::ViewportData> GetViewportData(const ViewportDescriptor* viewport) final override {
					const bool isTargetViewport = (viewport == m_gizmoContext->Viewport()->TargetSceneViewport());
					return isTargetViewport ? (const LightDescriptor::ViewportData*)this : m_darkLightDescriptor.operator->();
				}
				inline virtual LightInfo GetLightInfo()const override { return m_info; }
				inline virtual AABB GetLightBounds()const override {
					return AABB(Vector3(std::numeric_limits<float>::infinity()), Vector3(std::numeric_limits<float>::infinity()));
				}
			};
		};

		GizmoSceneDefaultLight::GizmoSceneDefaultLight(Scene::LogicContext* context) 
			: Component(context, "GizmoSceneDefaultLight") {
			m_targetSceneLights = LightDescriptor::Set::GetInstance(GizmoContext()->TargetContext());
			assert(m_targetSceneLights != nullptr);
		}

		GizmoSceneDefaultLight::~GizmoSceneDefaultLight() {
			Update();
		}

		void GizmoSceneDefaultLight::Update() {
			const bool onlyDefaultLights = [&]() {
				bool result = true;
				m_targetSceneLights->GetAll([&](const LightDescriptor* desc) {
					result &= (dynamic_cast<const Helpers::DefaultLightDescriptor*>(desc) != nullptr);
					});
				return result;
			}();

			if ((!Destroyed()) && onlyDefaultLights) {
				if (m_lightDescriptor != nullptr)
					return;
				uint32_t typeId;
				if (!Context()->Graphics()->Configuration().ShaderLoader()->GetLightTypeId("Jimara_HDRI_Light", typeId))
					return;
				const Reference<Graphics::TextureSampler> brdfIntegrationMap = HDRIEnvironment::BrdfIntegrationMap(
					Context()->Graphics()->Device(), Context()->Graphics()->Configuration().ShaderLoader());
				if (brdfIntegrationMap == nullptr)
					return;
				const Reference<const Graphics::BindlessSet<Graphics::TextureSampler>::Binding> brdfIntegrationMapBinding = 
					GizmoContext()->TargetContext()->Graphics()->Bindless().Samplers()->GetBinding(brdfIntegrationMap);
				if (brdfIntegrationMapBinding == nullptr)
					return;
				const Reference<const Graphics::ShaderClass::TextureSamplerBinding> whiteTexture =
					Graphics::ShaderClass::SharedTextureSamplerBinding(Vector4(1.0f), Context()->Graphics()->Device());
				if (whiteTexture == nullptr)
					return;
				const Reference<const Graphics::BindlessSet<Graphics::TextureSampler>::Binding> whiteTextureBinding =
					GizmoContext()->TargetContext()->Graphics()->Bindless().Samplers()->GetBinding(whiteTexture->BoundObject());
				if (whiteTextureBinding == nullptr)
					return;
				const Reference<LightDescriptor> lightDescriptor = Object::Instantiate<Helpers::DefaultLightDescriptor>(
					GizmoContext(), typeId, whiteTexture, whiteTextureBinding, brdfIntegrationMapBinding);
				m_lightDescriptor = Object::Instantiate<LightDescriptor::Set::ItemOwner>(lightDescriptor);
				m_targetSceneLights->Add(m_lightDescriptor);
			}
			else {
				if (m_lightDescriptor == nullptr)
					return;
				m_targetSceneLights->Remove(m_lightDescriptor);
				m_lightDescriptor = nullptr;
			}
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection GizmoSceneDefaultLight_GizmoConnection =
				Gizmo::ComponentConnection::Targetless<GizmoSceneDefaultLight>();
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::GizmoSceneDefaultLight>() {
		Editor::Gizmo::AddConnection(Editor::GizmoSceneDefaultLight_GizmoConnection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::GizmoSceneDefaultLight>() {
		Editor::Gizmo::RemoveConnection(Editor::GizmoSceneDefaultLight_GizmoConnection);
	}
}
