#include "CapsuleColliderGizmo.h"
#include <Components/Physics/CapsuleCollider.h>
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


namespace Jimara {
	namespace Editor {
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
						Reference<TriMesh> mesh = MeshContants::Tri::WireCapsule(radius, height);
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
