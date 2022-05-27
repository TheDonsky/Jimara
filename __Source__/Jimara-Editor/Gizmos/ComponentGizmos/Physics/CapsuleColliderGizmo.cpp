#include "CapsuleColliderGizmo.h"
#include <Components/Physics/CapsuleCollider.h>
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


namespace Jimara {
	namespace Editor {
		class CapsuleColliderGizmo::MeshCache : public virtual ObjectCache<uint64_t> {
		private:
#pragma warning(disable: 4250)
			class CapsuleMeshAsset
				: public virtual Asset::Of<TriMesh>
				, public virtual ObjectCache<uint64_t>::StoredObject {
				const float m_radius;
				const float m_height;

			public:
				inline CapsuleMeshAsset(float radius, float height) 
					: Asset(GUID::Generate()), m_radius(radius), m_height(height) {}

				inline ~CapsuleMeshAsset() {}

			protected:
				inline virtual Reference<TriMesh> LoadItem()final override {
					Reference<TriMesh> mesh = Object::Instantiate<TriMesh>();
					TriMesh::Writer writer(mesh);
					
					// Name:
					{
						std::stringstream stream;
						stream << "CapsuleColliderGizmo_Mesh[R=" << m_radius << "; H:" << m_height << "]";
						writer.Name() = stream.str();
					}

					// Connects vertices:
					auto connectVerts = [&](uint32_t a, uint32_t b) {
						writer.AddFace(TriangleFace(a, b, b));
					};

					// Creates ark:
					static const constexpr uint32_t arkDivisions = 32;
					auto addArk = [&](uint32_t first, uint32_t last, Vector3 center, Vector3 up, Vector3 right) {
						static const constexpr float angleStep = Math::Radians(360) / static_cast<float>(arkDivisions);
						auto addVert = [&](uint32_t vertId) {
							const float angle = angleStep * vertId;
							MeshVertex vertex = {};
							vertex.normal = (std::cos(angle) * right) + (std::sin(angle) * up);
							vertex.position = center + (vertex.normal * m_radius);
							vertex.uv = Vector2(0.5f);
							writer.AddVert(vertex);
						};
						addVert(first);
						for (uint32_t vertId = first + 1; vertId < last; vertId++) {
							addVert(vertId);
							connectVerts(writer.VertCount() - 2, writer.VertCount() - 1);
						}
					};

					// Create shape on given 'right' and 'forward' axis:
					const constexpr Vector3 up = Math::Up();
					auto createOutline = [&](Vector3 right) {
						const uint32_t base = writer.VertCount();
						addArk(0, (arkDivisions / 2) + 1, m_height * 0.5f * up, up, right);
						connectVerts(writer.VertCount() - 1, writer.VertCount());
						addArk(arkDivisions / 2, arkDivisions + 1, -m_height * 0.5f * up, up, right);
						connectVerts(writer.VertCount() - 1, base);
					};
					createOutline(Math::Right());
					createOutline(Math::Forward());

					// Create rings on top & bottom:
					auto createRing = [&](float elevation) {
						const uint32_t base = writer.VertCount();
						addArk(0, arkDivisions, elevation * up, Math::Right(), Math::Forward());
						connectVerts(writer.VertCount() - 1, base);
					};
					createRing(m_height * 0.5f);
					createRing(m_height * -0.5f);

					return mesh;
				}
			};
#pragma warning(default: 4250)

		public:
			inline static Reference<TriMesh> GetFor(float radius, float height) {
				static_assert(sizeof(float) == sizeof(uint32_t));
				static_assert((sizeof(uint32_t) << 1) == sizeof(uint64_t));
				uint64_t key;
				{
					float* words = reinterpret_cast<float*>(&key);
					words[0] = radius;
					words[1] = height;
				}
				static MeshCache cache;
				Reference<CapsuleMeshAsset> asset = cache.GetCachedOrCreate(key, false, [&]() -> Reference<CapsuleMeshAsset> {
					return Object::Instantiate<CapsuleMeshAsset>(radius, height);
					});
				return asset->Load();
			}
		};

		CapsuleColliderGizmo::CapsuleColliderGizmo(Scene::LogicContext* context)
			: Component(context, "CapsuleColliderGizmo")
			, m_renderer(Object::Instantiate<MeshRenderer>(
				Object::Instantiate<Transform>(this, "CapsuleColliderGizmo_Pose"), "CapsuleColliderGizmo_ShapeRenderer")) {
			const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), Vector3(0.0f, 1.0f, 0.0f));
			m_renderer->SetMaterialInstance(material);
			m_renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::OVERLAY));
			m_renderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
		}

		CapsuleColliderGizmo::~CapsuleColliderGizmo() {}

		void CapsuleColliderGizmo::Update() {
			Reference<CapsuleCollider> collider = Target<CapsuleCollider>();
			Reference<Transform> colliderTransform = (collider == nullptr ? nullptr : collider->GetTransfrom());
			if (colliderTransform == nullptr || (!collider->ActiveInHeirarchy()))
				m_renderer->SetEnabled(false);
			else {
				m_renderer->SetEnabled(true);

				// Scale:
				const Vector3 lossyScale = colliderTransform->LossyScale();
				float tipScale = Math::Max(lossyScale.x, Math::Max(lossyScale.y, lossyScale.z));

				// Update mesh:
				{
					float radius = collider->Radius() * tipScale;
					float height = collider->Height() * lossyScale.y;
					if (std::abs(radius) > std::numeric_limits<float>::epsilon()) {
						height /= radius;
						tipScale *= collider->Radius();
						radius = 1.0f;
					}
					if (m_lastRadius != radius || m_lastHeight != height) {
						Reference<TriMesh> mesh = MeshCache::GetFor(radius, height);
						m_renderer->SetMesh(mesh);
						m_lastRadius = radius;
						m_lastHeight = height;
					}
				}

				// Update transform:
				{
					Transform* poseTransform = m_renderer->GetTransfrom();
					poseTransform->SetWorldPosition(colliderTransform->WorldPosition());
					poseTransform->SetWorldEulerAngles(Math::EulerAnglesFromMatrix(colliderTransform->WorldRotationMatrix() * [&]() -> Matrix4 {
						Physics::CapsuleShape::Alignment alignment = collider->Alignment();
						if (alignment == Physics::CapsuleShape::Alignment::X) return Math::MatrixFromEulerAngles(Vector3(0.0f, 0.0f, 90.0f));
						else if (alignment == Physics::CapsuleShape::Alignment::Y) return Math::Identity();
						else if (alignment == Physics::CapsuleShape::Alignment::Z) return Math::MatrixFromEulerAngles(Vector3(90.0f, 0.0f, 0.0f));
						else return Math::Identity();
						}()));
					poseTransform->SetLocalScale(std::abs(m_lastRadius) > std::numeric_limits<float>::epsilon() ? Vector3(tipScale) : lossyScale);
				}
			}

			// __TODO__: Add handle controls...
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection ColliderGizmo_Connection =
				Gizmo::ComponentConnection::Make<CapsuleColliderGizmo, CapsuleCollider>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::CapsuleColliderGizmo>() {
		Editor::Gizmo::AddConnection(Editor::ColliderGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::CapsuleColliderGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::ColliderGizmo_Connection);
	}
}
