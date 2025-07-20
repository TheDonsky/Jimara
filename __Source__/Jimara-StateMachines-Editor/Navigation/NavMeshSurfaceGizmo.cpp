#include "NavMeshSurfaceGizmo.h"
#include <Jimara-StateMachines/Navigation/NavMesh/NavMeshSurface.h>
#include <Jimara/Data/Materials/PBR/PBR_Shader.h>

namespace Jimara {
	namespace Editor {
		NavMeshSurfaceGizmo::NavMeshSurfaceGizmo(Scene::LogicContext* context) 
			: Component(context, "NavMeshSurfaceGizmo") {
#pragma warning(disable: 4250)
			struct CachedMaterialAsset : public virtual Asset::Of<Material>, public virtual ObjectCache<Reference<const Object>>::StoredObject {
				const Reference<Scene::GraphicsContext> context;

				inline CachedMaterialAsset(Scene::GraphicsContext* ctx) : Asset(GUID::Generate()), context(ctx) {}
				inline virtual ~CachedMaterialAsset() {}
				inline virtual Reference<Material> LoadItem() {
					const Reference<Material> material = Object::Instantiate<Material>(
						context->Device(), context->Bindless().Buffers(), context->Bindless().Samplers());
					{
						Material::Writer writer(material);
						writer.SetShader(PBR_Shader::Transparent(context->Configuration().ShaderLibrary()->LitShaders()));
						writer.SetPropertyValue(PBR_Shader::ALBEDO_NAME, Vector4(0.0f, 0.0f, 0.0f, 0.125f));
						writer.SetPropertyValue(PBR_Shader::EMISSION_NAME, Vector3(0.25f));
						writer.SetPropertyValue(PBR_Shader::METALNESS_NAME, 0.0f);
						writer.SetPropertyValue(PBR_Shader::ROUGHNESS_NAME, 0.5f);
						writer.SetPropertyValue(PBR_Shader::ALPHA_THRESHOLD_NAME, 0.0f);
						writer.SetPropertyValue(PBR_Shader::TILING_NAME, Vector2(1.0f));
						writer.SetPropertyValue(PBR_Shader::OFFSET_NAME, Vector2(0.0f));
					}
					return material;
				}
			};
			struct MaterialCache : public virtual ObjectCache<Reference<const Object>> {
				static Reference<Material> GetMaterial(Scene::LogicContext* context) {
					static MaterialCache cache;
					return Reference<Asset::Of<Material>>(cache.GetCachedOrCreate(context->Graphics(), [&]() {
						return Object::Instantiate<CachedMaterialAsset>(context->Graphics());
						}))->Load();
				}
			};
#pragma warning(default: 4250)

			m_areaRenderer = Object::Instantiate<MeshRenderer>(
				Object::Instantiate<Transform>(this, "NavMeshSurfaceGizmo_Transform"), "NavMeshSurfaceGizmo_AreaRenderer");
			m_wireRenderer = Object::Instantiate<MeshRenderer>(m_areaRenderer->GetTransform(), "NavMeshSurfaceGizmo_WireRenderer");
			const Reference<Material> navMeshSurfaceMaterial = MaterialCache::GetMaterial(context);
			m_areaRenderer->SetLayer(static_cast<Layer>(GizmoLayers::OVERLAY));
			m_areaRenderer->SetMaterial(navMeshSurfaceMaterial);
			m_wireRenderer->SetLayer(static_cast<Layer>(GizmoLayers::WORLD_SPACE));
			m_wireRenderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
		}

		NavMeshSurfaceGizmo::~NavMeshSurfaceGizmo() {}

		void NavMeshSurfaceGizmo::Update() {
			const NavMeshSurface* surface = Target<NavMeshSurface>();
			const NavMesh::Surface* shape = (surface == nullptr) ? nullptr : surface->Surface();
			const Reference<const NavMesh::BakedSurfaceData> surfaceData = (shape == nullptr)
				? Reference<const NavMesh::BakedSurfaceData>() : shape->Data();
			Transform* const gizmoTransform = m_areaRenderer->GetTransform();
			if (surfaceData == nullptr || surfaceData->geometry == nullptr) {
				m_areaRenderer->SetMesh(nullptr);
				m_wireRenderer->SetMesh(nullptr);
				gizmoTransform->SetEnabled(false);
			}
			else {
				m_areaRenderer->SetMesh(surfaceData->geometry);
				m_wireRenderer->SetMesh(surfaceData->geometry);
				gizmoTransform->SetEnabled(true);
				const Transform* surfaceTransform = (surface == nullptr) ? nullptr : surface->GetTransform();
				if (surfaceTransform == nullptr) {
					gizmoTransform->SetLocalPosition(Vector3(0.0f));
					gizmoTransform->SetLocalEulerAngles(Vector3(0.0f));
					gizmoTransform->SetLocalScale(Vector3(1.0f));
				}
				else {
					gizmoTransform->SetLocalPosition(surfaceTransform->WorldPosition());
					gizmoTransform->SetLocalEulerAngles(surfaceTransform->WorldEulerAngles());
					gizmoTransform->SetLocalScale(surfaceTransform->LossyScale());
				}
			}
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::NavMeshSurfaceGizmo>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection =
			Editor::Gizmo::ComponentConnection::Make<Editor::NavMeshSurfaceGizmo, NavMeshSurface>();
		report(connection);
	}
}
