#include "BoxColliderGizmo.h"
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include <Jimara/Components/Physics/BoxCollider.h>
#include <Jimara/Data/Geometry/MeshConstants.h>
#include <Jimara/Data/Materials/SampleDiffuse/SampleDiffuseShader.h>


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
			Reference<Transform> colliderTransform = (collider == nullptr) ? nullptr : collider->GetTransform();
			if (colliderTransform != nullptr && collider->ActiveInHierarchy()) {
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
	}


	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::BoxColliderGizmo>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection = 
			Editor::Gizmo::ComponentConnection::Make<Editor::BoxColliderGizmo, BoxCollider>();
		report(connection);
	}
}
