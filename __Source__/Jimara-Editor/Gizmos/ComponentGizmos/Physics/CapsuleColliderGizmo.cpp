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
				Transform* poseTransform = m_renderer->GetTransfrom();
				poseTransform->SetWorldPosition(colliderTransform->WorldPosition());
				poseTransform->SetWorldEulerAngles(colliderTransform->WorldEulerAngles());
				// __TODO__: Resize capsule here...
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
