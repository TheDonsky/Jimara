#include "MeshColliderGizmo.h"
#include <Components/Physics/MeshCollider.h>
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


namespace Jimara {
	namespace Editor {
		MeshColliderGizmo::MeshColliderGizmo(Scene::LogicContext* context)
			: Component(context, "MeshColliderGizmo")
			, m_renderer(Object::Instantiate<MeshRenderer>(
				Object::Instantiate<Transform>(this, "MeshColliderGizmo_Pose"), "MeshColliderGizmo_ShapeRenderer")) {
			const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), Vector3(0.0f, 1.0f, 0.0f));
			m_renderer->SetMaterialInstance(material);
			m_renderer->SetLayer(static_cast<Layer>(GizmoLayers::WORLD_SPACE));
			m_renderer->SetGeometryType(Graphics::Experimental::GraphicsPipeline::IndexType::EDGE);
		}

		MeshColliderGizmo::~MeshColliderGizmo() {}

		void MeshColliderGizmo::Update() {
			Reference<MeshCollider> collider = Target<MeshCollider>();
			Reference<Transform> colliderTransform = (collider == nullptr ? nullptr : collider->GetTransfrom());
			if (colliderTransform == nullptr || (!collider->ActiveInHeirarchy()))
				m_renderer->SetEnabled(false);
			else {
				m_renderer->SetEnabled(true);
				m_renderer->SetMesh(collider->Mesh());
				Transform* poseTransform = m_renderer->GetTransfrom();
				poseTransform->SetWorldPosition(colliderTransform->WorldPosition());
				poseTransform->SetWorldEulerAngles(colliderTransform->WorldEulerAngles());
				poseTransform->SetLocalScale(colliderTransform->LossyScale());
			}
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection ColliderGizmo_Connection =
				Gizmo::ComponentConnection::Make<MeshColliderGizmo, MeshCollider>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::MeshColliderGizmo>() {
		Editor::Gizmo::AddConnection(Editor::ColliderGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::MeshColliderGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::ColliderGizmo_Connection);
	}
}
