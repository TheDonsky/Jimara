#include "MeshRendererGizmo.h"



namespace Jimara {
	namespace Editor {
		MeshRendererGizmo::MeshRendererGizmo(Scene::LogicContext* context) 
			: Component(context, "MeshRendererGizmo")
			, m_wireframeRenderer(
				Object::Instantiate<MeshRenderer>(
					Object::Instantiate<Transform>(this, "MeshRendererGizmo_Transform"), 
					"MeshRendererGizmo_Renderer")) {
			m_wireframeRenderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::WORLD_SPACE));
			m_wireframeRenderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
		}

		MeshRendererGizmo::~MeshRendererGizmo() {}

		namespace {
#pragma warning(disable: 4250)
			class BeveledMeshAsset : public virtual Asset::Of<TriMesh>, public virtual ObjectCache<Reference<const Object>>::StoredObject {
			private:
				const Reference<const TriMesh> m_mesh;

				class BeveledMesh : public virtual TriMesh {
				private:
					const Reference<const TriMesh> m_mesh;

					inline void Update(const TriMesh* mesh) {
						TriMesh::Reader reader(mesh);
						TriMesh::Writer writer(this);
						{
							while (writer.VertCount() > reader.VertCount()) writer.PopVert();
							while (writer.VertCount() < reader.VertCount()) writer.AddVert(MeshVertex());
							for (uint32_t i = 0; i < reader.VertCount(); i++) {
								Vertex& vertex = writer.Vert(i);
								vertex = reader.Vert(i);
								vertex.position += vertex.normal * 0.025f;
							}
						}
						{
							while (writer.FaceCount() > reader.FaceCount()) writer.PopFace();
							while (writer.FaceCount() < reader.FaceCount()) writer.AddFace(TriangleFace());
							for (uint32_t i = 0; i < reader.FaceCount(); i++) writer.Face(i) = reader.Face(i);
						}
					}

				public:
					inline BeveledMesh(const TriMesh* mesh) : TriMesh("BeveledMesh"), m_mesh(mesh) {
						m_mesh->OnDirty() += Callback(&BeveledMesh::Update, this);
						Update(m_mesh);
					}

					inline virtual ~BeveledMesh() {
						m_mesh->OnDirty() -= Callback(&BeveledMesh::Update, this);
					}
				};

			public:
				inline BeveledMeshAsset(const TriMesh* srcMesh) : Asset(GUID::Generate()), m_mesh(srcMesh) {}

				inline virtual ~BeveledMeshAsset() {}

				inline virtual Reference<TriMesh> LoadItem()override {
					if (m_mesh == nullptr) return nullptr;
					else return Object::Instantiate<BeveledMesh>(m_mesh);
				}

				class Cache : public virtual ObjectCache<Reference<const Object>> {
				public:
					inline static Reference<TriMesh> GetFor(const TriMesh* mesh) {
						static Cache cache;
						Reference<BeveledMeshAsset> asset = cache.GetCachedOrCreate(mesh, false, [&]() { return Object::Instantiate<BeveledMeshAsset>(mesh); });
						return asset->Load();
					}
				};
			};
#pragma warning(default: 4250)
		}

		void MeshRendererGizmo::Update() {
			const MeshRenderer* const target = Target<MeshRenderer>();
			if (target == nullptr) {
				m_wireframeRenderer->SetMesh(nullptr);
				return;
			}
			const Transform* const targetTransform = target->GetTransfrom();
			{
				m_wireframeRenderer->SetEnabled(target->ActiveInHeirarchy() && (targetTransform != nullptr));
				const Reference<TriMesh> mesh = BeveledMeshAsset::Cache::GetFor(target->Mesh());
				m_wireframeRenderer->SetMesh(mesh);
			}
			if (targetTransform != nullptr) {
				Transform* const wireTransform = m_wireframeRenderer->GetTransfrom();
				wireTransform->SetLocalPosition(targetTransform->WorldPosition());
				wireTransform->SetLocalEulerAngles(targetTransform->WorldEulerAngles());
				wireTransform->SetLocalScale(targetTransform->LossyScale());
			}
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection MeshRendererGizmo_Connection =
				Gizmo::ComponentConnection::Make<MeshRendererGizmo, MeshRenderer>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::MeshRendererGizmo>() {
		Editor::Gizmo::AddConnection(Editor::MeshRendererGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::MeshRendererGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::MeshRendererGizmo_Connection);
	}
}

