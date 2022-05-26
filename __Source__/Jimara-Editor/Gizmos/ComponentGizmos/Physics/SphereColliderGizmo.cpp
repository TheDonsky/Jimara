#include "SphereColliderGizmo.h"
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Components/Physics/SphereCollider.h>
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


namespace Jimara {
	namespace Editor {
		SphereColliderGizmo::SphereColliderGizmo(Scene::LogicContext* context)
			: Component(context, "SphereColliderGizmo")
			, m_poseTransform(Object::Instantiate<Transform>(this, "SphereColliderGizmo_Pose")) {
			const Reference<TriMesh> shape = MeshContants::Tri::Sphere();
			const Reference<Material::Instance> material = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device(), Vector3(0.0f, 1.0f, 0.0f));
			const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(m_poseTransform, "SphereColliderGizmo_ShapeRenderer", shape);
			renderer->SetMaterialInstance(material);
			renderer->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::OVERLAY));
			renderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
		}

		SphereColliderGizmo::~SphereColliderGizmo() {}

		void SphereColliderGizmo::Update() {
			Reference<SphereCollider> collider = Target<SphereCollider>();
			Reference<Transform> colliderTransform = (collider == nullptr ? nullptr : collider->GetTransfrom());
			if (colliderTransform == nullptr || (!collider->ActiveInHeirarchy()))
				m_poseTransform->SetEnabled(false);
			else {
				m_poseTransform->SetEnabled(true);
				m_poseTransform->SetWorldPosition(colliderTransform->WorldPosition());
				m_poseTransform->SetWorldEulerAngles(colliderTransform->WorldEulerAngles());
				m_poseTransform->SetLocalScale(colliderTransform->LossyScale() * collider->Radius());
			}

			// __TODO__: Add handle controls...
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection ColliderGizmo_Connection =
				Gizmo::ComponentConnection::Make<SphereColliderGizmo, SphereCollider>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::SphereColliderGizmo>() {
		Editor::Gizmo::AddConnection(Editor::ColliderGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SphereColliderGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::ColliderGizmo_Connection);
	}
}
