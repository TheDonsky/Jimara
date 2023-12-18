#include "BoxColliderGizmo.h"
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Components/Physics/BoxCollider.h>
#include <Data/Geometry/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


namespace Jimara {
	namespace Editor {
		BoxColliderGizmo::BoxColliderGizmo(Scene::LogicContext* context)
			: Component(context, "BoxColliderGizmo")
			, m_resizeHandle(Object::Instantiate<BoxResizeHandle>(this, Vector3(0.0f, 1.0f, 0.0f))) {
			m_resizeHandle->SetEnabled(false);
		}

		BoxColliderGizmo::~BoxColliderGizmo() {}

		void BoxColliderGizmo::Update() {
			Reference<BoxCollider> collider = Target<BoxCollider>();
			Reference<Transform> colliderTransform = (collider == nullptr) ? nullptr : collider->GetTransfrom();
			if (colliderTransform != nullptr && collider->ActiveInHeirarchy()) {
				m_resizeHandle->SetEnabled(true);
				const Vector3 initialSize = collider->Size();
				const Vector3 lossyScale = colliderTransform->LossyScale();
				const Vector3 initialScaledSize = lossyScale * initialSize;
				Vector3 scaledSize = initialScaledSize;
				m_resizeHandle->Update(colliderTransform->WorldPosition(), colliderTransform->WorldEulerAngles(), scaledSize);
				if (initialScaledSize != scaledSize) {
					auto safeDivide = [](float a, float b, float c) { return (std::abs(b) > std::numeric_limits<float>::epsilon()) ? (a / b) : c; };
					collider->SetSize(Vector3(
						safeDivide(scaledSize.x, lossyScale.x, initialSize.x),
						safeDivide(scaledSize.y, lossyScale.y, initialSize.y),
						safeDivide(scaledSize.z, lossyScale.z, initialSize.z)));
				}
			}
			else m_resizeHandle->SetEnabled(false);
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection ColliderGizmo_Connection =
				Gizmo::ComponentConnection::Make<BoxColliderGizmo, BoxCollider>();
		}
	}


	template<> void TypeIdDetails::OnRegisterType<Editor::BoxColliderGizmo>() {
		Editor::Gizmo::AddConnection(Editor::ColliderGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::BoxColliderGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::ColliderGizmo_Connection);
	}
}
