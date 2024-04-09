#include "MeshColliderGizmo.h"
#include <Jimara/Components/Physics/MeshCollider.h>
#include <Jimara/Data/Geometry/MeshConstants.h>
#include <Jimara/Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


namespace Jimara {
	namespace Editor {
		MeshColliderGizmo::MeshColliderGizmo(Scene::LogicContext* context)
			: Component(context, "MeshColliderGizmo")
			, m_renderer(Object::Instantiate<MeshRenderer>(
				Object::Instantiate<Transform>(this, "MeshColliderGizmo_Pose"), "MeshColliderGizmo_ShapeRenderer")) {
			const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), Vector3(0.0f, 1.0f, 0.0f));
			m_renderer->SetMaterialInstance(material);
			m_renderer->SetLayer(static_cast<Layer>(GizmoLayers::WORLD_SPACE));
			m_renderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
		}

		MeshColliderGizmo::~MeshColliderGizmo() {}

		void MeshColliderGizmo::Update() {
			Reference<MeshCollider> collider = Target<MeshCollider>();
			Reference<Transform> colliderTransform = (collider == nullptr ? nullptr : collider->GetTransfrom());
			if (colliderTransform == nullptr || (!collider->ActiveInHierarchy()))
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
	}


	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::MeshColliderGizmo>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection =
			Editor::Gizmo::ComponentConnection::Make<Editor::MeshColliderGizmo, MeshCollider>();
		report(connection);
	}
}
