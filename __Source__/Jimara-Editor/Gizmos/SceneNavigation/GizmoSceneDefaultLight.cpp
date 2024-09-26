#include "GizmoSceneDefaultLight.h"
#include <Jimara/Environment/Rendering/ImageBasedLighting/HDRIEnvironment.h>
#include <Jimara/Math/Random.h>


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
				const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> m_whiteTexture;
				const Reference<const Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_whiteTextureBinding;
				const Reference<const Graphics::BindlessSet<Graphics::TextureSampler>::Binding> m_brdfIntegrationMapBinding;
				const LightData m_data;
				const LightInfo m_info;

			public:
				inline DefaultLightDescriptor(
					GizmoScene::Context* gizmoContext, uint32_t typeId,
					const Graphics::ResourceBinding<Graphics::TextureSampler>* whiteTexture,
					const Graphics::BindlessSet<Graphics::TextureSampler>::Binding* whiteTextureBinding,
					const Graphics::BindlessSet<Graphics::TextureSampler>::Binding* brdfIntegrationMapBinding)
					: m_gizmoContext(gizmoContext)
					, m_whiteTexture(whiteTexture)
					, m_whiteTextureBinding(whiteTextureBinding)
					, m_brdfIntegrationMapBinding(brdfIntegrationMapBinding)
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
					}()) {
					assert(m_gizmoContext != nullptr);
					assert(m_whiteTexture != nullptr);
					assert(m_whiteTextureBinding != nullptr);
					assert(m_whiteTextureBinding->BoundObject() == m_whiteTexture->BoundObject());
					assert(m_brdfIntegrationMapBinding != nullptr);
				}
				inline virtual ~DefaultLightDescriptor() {}

				inline virtual Reference<const LightDescriptor::ViewportData> GetViewportData(const ViewportDescriptor* viewport) final override {
					return (viewport == m_gizmoContext->Viewport()->TargetSceneViewport()) ? this : nullptr;
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
				if (!Context()->Graphics()->Configuration().ShaderLibrary()->GetLightTypeId("Jimara_HDRI_Light", typeId))
					return;
				const Reference<Graphics::TextureSampler> brdfIntegrationMap = HDRIEnvironment::BrdfIntegrationMap(
					Context()->Graphics()->Device(), Context()->Graphics()->Configuration().ShaderLibrary());
				if (brdfIntegrationMap == nullptr)
					return;
				const Reference<const Graphics::BindlessSet<Graphics::TextureSampler>::Binding> brdfIntegrationMapBinding = 
					GizmoContext()->TargetContext()->Graphics()->Bindless().Samplers()->GetBinding(brdfIntegrationMap);
				if (brdfIntegrationMapBinding == nullptr)
					return;
				const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> whiteTexture =
					Graphics::SharedTextureSamplerBinding(Vector4(1.0f), Context()->Graphics()->Device());
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
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::GizmoSceneDefaultLight>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection = 
			Editor::Gizmo::ComponentConnection::Targetless<Editor::GizmoSceneDefaultLight>();
		report(connection);
	}
}
