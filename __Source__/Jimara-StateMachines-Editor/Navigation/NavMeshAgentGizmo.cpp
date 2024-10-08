#include "NavMeshAgentGizmo.h"
#include <Jimara-StateMachines/Navigation/NavMesh/NavMeshAgent.h>
#include <Jimara/Data/Materials/PBR/PBR_Shader.h>


namespace Jimara {
	namespace Editor {
		NavMeshAgentGizmo::NavMeshAgentGizmo(Scene::LogicContext* context)
			: Component(context, "NavMeshAgentGizmo")
			, m_pathRenderer(Object::Instantiate<MeshRenderer>(this, "NavMeshSurfaceGizmo_AreaRenderer")) {
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
						writer.SetPropertyValue(PBR_Shader::EMISSION_NAME, Vector3(0.0f, 0.5f, 0.0f));
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
			const Reference<Material> material = MaterialCache::GetMaterial(context);
			const Reference<TriMesh> mesh = Object::Instantiate<TriMesh>();
			m_pathRenderer->SetLayer(static_cast<Layer>(GizmoLayers::OVERLAY));
			m_pathRenderer->SetMaterial(material);
			m_pathRenderer->SetMesh(mesh);
			//m_pathRenderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
		}

		NavMeshAgentGizmo::~NavMeshAgentGizmo() {}

		void NavMeshAgentGizmo::Update() {
			const NavMeshAgent* agent = Target<NavMeshAgent>();
			if (agent == nullptr) {
				m_pathRenderer->SetEnabled(false);
				return;
			}
			else if (!m_pathRenderer->Enabled())
				m_pathRenderer->SetEnabled(true);

			const std::shared_ptr<const std::vector<NavMesh::PathNode>> pathPtr = agent->Path();
			const std::vector<NavMesh::PathNode>& path = *pathPtr;
			const float radius = agent->Radius();
			TriMesh::Writer mesh(m_pathRenderer->Mesh());
			
			while (mesh.FaceCount() > 0u)
				mesh.PopFace();
			while (mesh.VertCount() > 0u)
				mesh.PopVert();

			for (size_t i = 1u; i < path.size(); i++) {
				const NavMesh::PathNode a = path[i - 1u];
				const NavMesh::PathNode b = path[i];
				const Vector3 rightA = Math::Normalize(Math::Cross(b.position - a.position, a.normal));
				const Vector3 rightB = Math::Normalize(Math::Cross(b.position - a.position, b.normal));
				const uint32_t baseVert = mesh.VertCount();
				mesh.AddVert(MeshVertex(a.position + rightA * radius + a.normal * radius * 0.25f, a.normal));
				mesh.AddVert(MeshVertex(a.position - rightA * radius + a.normal * radius * 0.25f, a.normal));
				mesh.AddVert(MeshVertex(b.position + rightB * radius + b.normal * radius * 0.25f, b.normal));
				mesh.AddVert(MeshVertex(b.position - rightB * radius + b.normal * radius * 0.25f, b.normal));
				mesh.AddFace(TriangleFace(baseVert, baseVert + 1u, baseVert + 2u));
				mesh.AddFace(TriangleFace(baseVert + 1u, baseVert + 3u, baseVert + 2u));
			}
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::NavMeshAgentGizmo>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection =
			Editor::Gizmo::ComponentConnection::Make<Editor::NavMeshAgentGizmo, NavMeshAgent>();
		report(connection);
	}
}
